
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

power_pkt wemo_pkt; //buffer of wemo power_samples for transmission

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
void core_transmit_power_packet(void){
  int r, i, j;
  char tx_buffer[1100], content[500];
  char vrms_str[100], irms_str[100],  watts_str[100], pavg_str[100];
  char pf_str[100],   freq_str[100],    kwh_str[100];
  char *bufs[7] = {vrms_str, irms_str, watts_str, pavg_str, pf_str, freq_str, kwh_str};
  int32_t *srcs[7] = {wemo_pkt.vrms, wemo_pkt.irms, wemo_pkt.pavg, wemo_pkt.watts,
		    wemo_pkt.pf, wemo_pkt.freq, wemo_pkt.kwh};
  char *buf; int32_t *src;
  //now we are filling up the POST request
  wemo_pkt.status = POWER_PKT_TX_IN_PROG;
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
	  "6CA2",wemo_config.wemo_ip,wemo_pkt.timestamp,vrms_str,
	  irms_str,watts_str,pavg_str,pf_str,freq_str,kwh_str);
  sprintf(tx_buffer,"POST /config/plugs/log HTTP/1.1\r\nUser-Agent: WemoPlug\r\nHost: 18.111.15.238\r\nAccept:*/*\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-Type: application/json\r\n\r\n%s",strlen(content),content);
  //send the packet!
  r = wifi_transmit("18.111.15.238",80,tx_buffer);
  if(r==TX_SUCCESS){
    wemo_pkt.status = POWER_PKT_EMPTY;
  } else {
    wemo_pkt.status = POWER_PKT_TX_FAIL;
  }
  if(r==TX_ERROR){
    printf("resetting wifi\n");
    wifi_init();
  }
}

void core_log_power_data(power_sample *sample){
  static int wemo_pkt_idx = 0;
  //check for error conditions
  switch(wemo_pkt.status){
  case POWER_PKT_EMPTY:
    if(wemo_pkt_idx!=0){
      printf("Error, empty packet but nonzero index\n");
      return;
    }
    //set the timestamp for the packet with the first one
    rtc_get_time_str(wemo_pkt.timestamp);
    break;
  case POWER_PKT_TX_IN_PROG:
    printf("Error, this packet is still being transmitted\n");
    return;
  case POWER_PKT_TX_FAIL:
    printf("Prev TX failed... sorry :(\n");
  }
  wemo_pkt.status = POWER_PKT_FILLING;
  wemo_pkt.vrms[wemo_pkt_idx]  = sample->vrms;
  wemo_pkt.irms[wemo_pkt_idx]  = sample->irms;
  wemo_pkt.watts[wemo_pkt_idx] = sample->watts;
  wemo_pkt.pavg[wemo_pkt_idx]  = sample->pavg;
  wemo_pkt.freq[wemo_pkt_idx]  = sample->freq;
  wemo_pkt.pf[wemo_pkt_idx]    = sample->pf;
  wemo_pkt.kwh[wemo_pkt_idx]   = sample->kwh;
  wemo_pkt_idx++;
  if(wemo_pkt_idx==PKT_SIZE){
    core_transmit_power_packet();
    wemo_pkt_idx = 0;
  }
}

/***** Kernel monitor command interpreter *****/

/****************
 * This is the core loop
 * Uses a tick to schedule operations
 *****************/
uint8_t sys_tick=0;

void monitor(void){
  //setup the system tick
  //use Timer0 channel 1
  pmc_enable_periph_clk(ID_TC1);
  tc_init(TC0, 1,                        // channel 1
	  TC_CMR_TCCLKS_TIMER_CLOCK5     // source clock (CLOCK5 = Slow Clock)
	  | TC_CMR_CPCTRG                // up mode with automatic reset on RC match
	  | TC_CMR_WAVE                  // waveform mode
	  | TC_CMR_ACPA_CLEAR            // RA compare effect: clear
	  | TC_CMR_ACPC_SET              // RC compare effect: set
	  );
  TC0->TC_CHANNEL[1].TC_RA = 0;  // doesn't matter
  TC0->TC_CHANNEL[1].TC_RC = 32000;  // sets frequency: 32kHz/32000 = 1 Hz
  NVIC_EnableIRQ(TC1_IRQn);
  tc_enable_interrupt(TC0, 1, TC_IER_CPCS);
  tc_start(TC0,1);
  uint8_t prev_tick=0;
  //initialize the power packet
  wemo_pkt.status = POWER_PKT_EMPTY;
  while (1) {
    if(sys_tick!=prev_tick){ 
      prev_tick=sys_tick; //ticks are slow enough that we don't need mutex
      //check if there is a valid data packet
      if(wemo_sample.valid==true){
	core_log_power_data(&wemo_sample);
      }

      server_read_power();
    }
    // don't worry about reading in commands right now
    //buf = readline();
    //if (buf != NULL)
    //  runcmd(buf);
  }
}


ISR(TC1_Handler)
{
  sys_tick++;
  gpio_toggle_pin(BUTTON_PIN);
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
