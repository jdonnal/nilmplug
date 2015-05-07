
/****************************
 * Monitor code from 6.828
 *
 ***************************/
#include "asf.h"
#include "debug.h"
#include "string.h"
#include "iv_adc.h"
#include "iv_dacc.h"
#include "usb.h"
#include "types.h"
#include "readline.h"
#include "monitor.h"
#include "rtc.h"
#include "server.h"
#include "wifi.h"
#include "wemo_fs.h"

#define CMDBUF_SIZE	80	// enough for one VGA text line


//ping pong packet structs for filling and transmitting
power_pkt power_pkts[2];
power_pkt *cur_pkt, *tx_pkt;

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv);
};

static struct Command commands[] = {
	{ "hello", "test command", mon_test },
	{ "set_rtc", "set RTC with unix timestamp", mon_set_rtc},
	{ "get_rtc", "get RTC as unit timestamp", mon_get_rtc},
	{ "relay_on", "turn relay on",mon_relay_on},
	{ "relay_off", "turn relay off",mon_relay_off}
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

int
mon_test(int argc, char **argv){
  print ("[monitor] hello world!\n");
  return 0;
}

int
mon_set_rtc(int argc, char **argv){
  if(argc!=8){
    print("wrong number of args to set time\n");
    return -1;
  }
  uint8_t yr = atoi(argv[1]);
  uint8_t mo = atoi(argv[2]);
  uint8_t dt = atoi(argv[3]);
  uint8_t dw = atoi(argv[4]);
  uint8_t hr = atoi(argv[5]);
  uint8_t mn = atoi(argv[6]);
  uint8_t sc = atoi(argv[7]);

  if(i2c_rtc_set_time(sc,mn,hr,dw,dt,mo,yr)!=0)
    print("error setting RTC\n");
  return 0;
}

int
mon_get_rtc(int argc, char **argv){
  //char buf [30];
  //uint32_t val;
  //  val = i2c_rtc_get_time();
  print("TODO: reading RTC");
  //sprintf(buf,"%lu\n",val);
  //udi_cdc_write_buf(&buf,strlen(buf));
  return 0;
}

int
mon_relay_on(int argc, char **argv){
  gpio_set_pin_high(RELAY_PIN);
  return 0;
}

int
mon_relay_off(int argc, char **argv){
  gpio_set_pin_low(RELAY_PIN);
  return 0;
}

int
mon_relay_toggle(int argc, char **argv){
  gpio_toggle_pin(RELAY_PIN);
  return 0;
}

/***** Core commands ****/
void core_transmit_power_packet(power_pkt *wemo_pkt){
  int r, i, j;
  char tx_buffer[1100], content[500];
  char vrms_str[100], irms_str[100],  watts_str[100], pavg_str[100];
  char pf_str[100],   freq_str[100],    kwh_str[100];
  char *bufs[7] = {vrms_str, irms_str, watts_str, pavg_str, pf_str, freq_str, kwh_str};
  int32_t *srcs[7] = {wemo_pkt->vrms, wemo_pkt->irms, wemo_pkt->pavg, wemo_pkt->watts,
		    wemo_pkt->pf, wemo_pkt->freq, wemo_pkt->kwh};
  char *buf; int32_t *src;
  //make sure all the data is present
  if(wemo_pkt->status != POWER_PKT_READY){
    printf("Error, packet is not ready to tx!\n");
    return;
  }
  //now we are filling up the POST request
  wemo_pkt->status = POWER_PKT_TX_IN_PROG;
  //print out the data arrays as strings
  for(j=0;j<7;j++){
    buf = bufs[j]; src = srcs[j];
    memset(buf,0x0,100); //clear out the buffers
    for(i=0;i<PKT_SIZE;i++){
      sprintf(buf+strlen(buf),"%li",src[i]);
      if(i<(PKT_SIZE-1))
	sprintf(buf+strlen(buf),", ");
    }
  }
  //stick them in a json format
  memset(content,0x0,500); memset(tx_buffer,0x0,1100);
  sprintf(content,"{\"plug\":\"%s\",\"ip\":\"%s\",\"time\":\"%s\",\"vrms\":[%s],\"irms\":[%s],\"watts\":[%s],\"pavg\":[%s],\"pf\":[%s],\"freq\":[%s],\"kwh\":[%s]}",
	  "6CA2",wemo_config.wemo_ip,wemo_pkt->timestamp,vrms_str,
	  irms_str,watts_str,pavg_str,pf_str,freq_str,kwh_str);
  sprintf(tx_buffer,"POST /config/plugs/log HTTP/1.1\r\nUser-Agent: WemoPlug\r\nHost: 18.111.15.238\r\nAccept:*/*\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-Type: application/json\r\n\r\n%s",strlen(content),content);
  //send the packet!
  r = wifi_transmit("18.111.15.238",80,tx_buffer);
  if(r==TX_SUCCESS){
    wemo_pkt->status = POWER_PKT_EMPTY;
  } else {
    wemo_pkt->status = POWER_PKT_TX_FAIL;
  }
}

/////////////////////////
// core_log_power_data
//     Add a power_sample to the current power_pkt
//     When the pkt is full switch to the other pkt
//     (ping pong buffer). Throws an error if the 
//     next buffer is not empty (still being TX'd)
void core_log_power_data(power_sample *sample){
  static int wemo_sample_idx = 0;
  power_pkt *tmp_pkt;
  //check for error conditions
  switch(cur_pkt->status){
  case POWER_PKT_EMPTY:
    if(wemo_sample_idx!=0){
      printf("Error, empty packet but nonzero index\n");
      return;
    }
    //set the timestamp for the packet with the first one
    rtc_get_time_str(cur_pkt->timestamp);
    break;
  case POWER_PKT_TX_IN_PROG:
    printf("Error, this packet is still being transmitted\n");
    return;
  case POWER_PKT_TX_FAIL:
    printf("Error, this packet failed TX and isn't clean\n");
    break;
  case POWER_PKT_READY:
    printf("Transmit this packet first!\n");
    return;
  }
  cur_pkt->status = POWER_PKT_FILLING;
  cur_pkt->vrms[wemo_sample_idx]  = sample->vrms;
  cur_pkt->irms[wemo_sample_idx]  = sample->irms;
  cur_pkt->watts[wemo_sample_idx] = sample->watts;
  cur_pkt->pavg[wemo_sample_idx]  = sample->pavg;
  cur_pkt->freq[wemo_sample_idx]  = sample->freq;
  cur_pkt->pf[wemo_sample_idx]    = sample->pf;
  cur_pkt->kwh[wemo_sample_idx]   = sample->kwh;
  wemo_sample_idx++;
  if(wemo_sample_idx==PKT_SIZE){
    cur_pkt->status=POWER_PKT_READY;
    //switch buffers
    tmp_pkt = cur_pkt;
    cur_pkt = tx_pkt;
    tx_pkt = tmp_pkt;
    //start filling the new buffer
    wemo_sample_idx = 0;
  }
}

/***** Kernel monitor command interpreter *****/

/****************
 * This is the core loop
 * Uses a tick to schedule operations
 *****************/
uint8_t sys_tick=0;

void monitor(void){
  //User PWM for system tick
  pmc_enable_periph_clk(ID_PWM);
  pwm_channel_disable(PWM, 0);
  pwm_clock_t clock_setting = {
    .ul_clka = 1000, //1 kHz
    .ul_clkb = 0, //not used
    .ul_mck = sysclk_get_cpu_hz()
  };
  pwm_init(PWM, &clock_setting);
  //turn on channel 0
  pwm_channel_t channel = {
    .channel = 0,
    .ul_duty = 0,
    .ul_period = 5000, //sample every 5 seconds
    .ul_prescaler = PWM_CMR_CPRE_CLKA,
    .polarity = PWM_HIGH,
  };
  pwm_channel_init(PWM, &channel);
  //enable interrupts on overflow
  pwm_channel_enable_interrupt(PWM,0,0);
  pwm_channel_enable(PWM,0);
  NVIC_EnableIRQ(PWM_IRQn);

  //initialize the power packet buffers
  tx_pkt = &power_pkts[0];
  cur_pkt = &power_pkts[1];
  //both are empty
  tx_pkt->status = POWER_PKT_EMPTY;
  cur_pkt->status = POWER_PKT_EMPTY;
  while (1) {

    if(tx_pkt->status==POWER_PKT_READY){
      //try to send the packet 
      core_transmit_power_packet(tx_pkt);
      if(tx_pkt->status==POWER_PKT_TX_FAIL){
	printf("resetting wifi\n");
	wifi_init();
	//try again (only time for 2 tries)
	tx_pkt->status=POWER_PKT_READY;
	core_transmit_power_packet(tx_pkt);
      }
      //clear out the packet so we can start again
      tx_pkt->status=POWER_PKT_EMPTY;
    }
    // don't worry about reading in commands right now
    //buf = readline();
    //if (buf != NULL)
    //  runcmd(buf);
  }
}

ISR(PWM_Handler)
{
  sys_tick++;
  gpio_toggle_pin(BUTTON_PIN);
  //check if there is a valid wemo sample
  if(wemo_sample.valid==true){
    core_log_power_data(&wemo_sample);
  }
  //take a wemo reading
  server_read_power();
  //clear the interrupt so we don't get stuck here
  pwm_channel_get_interrupt_status(PWM);  
}

ISR(TC1_Handler)
{
  //  sys_tick++;
  //gpio_toggle_pin(BUTTON_PIN);
  //clear the interrupt so we don't get stuck here
  tc_get_status(TC0,1);
}


#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf)
{
	int argc;
	char *argv[MAXARGS];
	uint16_t i;
	char p_buf[40];
	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			print("Too many arguments (max 16)\n");
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv);
	}
	sprintf(p_buf,"Unknown command '%s'\n", argv[0]);
	print(p_buf);
	return 0;
}
