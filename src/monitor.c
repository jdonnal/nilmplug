
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

#define CMDBUF_SIZE	80	// enough for one VGA text line


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
void core_log_power_data(power_pkt *pkt){
  int r;
  char content [500];
  char tx_buffer[1100];

  //  printf("%.2fV %.2fI %fW %.2fW %.2fpf %.2fHz %.2fkWh\n",
  //	 vrms,irms,watts,pavg,pf,freq,kwh);
  sprintf(content,"vrms=%li&irms=%li&watts=%li&pavg=%li&pf=%li&freq=%li&kwh=%li",
	  pkt->vrms,pkt->irms,pkt->watts,pkt->pavg,pkt->pf,pkt->freq,pkt->kwh);
  sprintf(tx_buffer,"POST /config/plugs/log HTTP/1.1\r\nUser-Agent: WemoPlug\r\nHost: 18.111.15.238\r\nAccept:*/*\r\nConnection: keep-alive\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n%s",strlen(content),content);
  r = wifi_transmit("18.111.15.238",80,tx_buffer);
  if(r==TX_ERROR){
    printf("resetting wifi\n");
    wifi_init();
  }
}

/***** Kernel monitor command interpreter *****/

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
  TC0->TC_CHANNEL[1].TC_RC = 64000;  // sets frequency: 32kHz/32000 = 1 Hz
  NVIC_EnableIRQ(TC1_IRQn);
  tc_enable_interrupt(TC0, 1, TC_IER_CPCS);
  tc_start(TC0,1);
  uint8_t prev_tick=0;
  //  char *buf;
  while (1) {
    if(sys_tick!=prev_tick){ 
      prev_tick=sys_tick; //ticks are slow enough that we don't need mutex

      //check if there is a valid data packet
      if(wemo_power.valid==true){
	core_log_power_data(&wemo_power);
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
  //clear the interrupt so we don't get stuck here
  tc_get_status(TC0,1);
}
