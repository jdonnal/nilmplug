
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
