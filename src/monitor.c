
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
  char buf [30];
  uint32_t val;
  //  val = i2c_rtc_get_time();
  print("reading RTC");
  sprintf(buf,"%lu\n",val);
  udi_cdc_write_buf(&buf,strlen(buf));
  return 0;
}

int
mon_relay_on(int argc, char **argv){
  gpio_set_pin_high(RELAY_PIN);
}

int
mon_relay_off(int argc, char **argv){
  gpio_set_pin_low(RELAY_PIN);
}

int
mon_relay_toggle(int argc, char **argv){
  gpio_toggle_pin(RELAY_PIN);
}

/***** Core commands ****/
void core_log_power_data(power_pkt *pkt){
  float vrms, irms, watts, pavg, pf, freq, kwh;
  char content [500];
  char tx_buffer[1000];
  vrms = pkt->vrms*0.001;
  irms = pkt->irms*7.77e-6;
  watts = pkt->watts*0.005;
  pavg = pkt->pavg*0.005;
  pf = pkt->pf*1e-3;
  freq = pkt->freq*1e-3;
  kwh = pkt->kwh*1e-3;

  printf("%.2fV %.2fI %fW %.2fW %.2fpf %.2fHz %.2fkWh\n",
	 vrms,irms,watts,pavg,pf,freq,kwh);
  sprintf(content,"vrms=%d&irms=%d&watts=%d&pavg=%d&pf=%d&freq=%d&kwh=%d",
	  pkt->vrms,pkt->irms,pkt->watts,pkt->pavg,pkt->pf,pkt->freq,pkt->kwh);
  sprintf(tx_buffer,"POST /plugs/log HTTP/1.0\r\nUser-Agent: WemoPlug\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d \r\n %s\r\n",
	  strlen(content),content);
  wifi_transmit("http://nilm4444.vpn.wattsworth.net",5000,tx_buffer);
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

void monitor(void){
  char *buf;
  while (1) {
    buf = readline();
    if (buf != NULL)
      runcmd(buf);
  }
}
