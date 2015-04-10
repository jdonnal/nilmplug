#include "asf.h"
#include "readline.h"
#include "usb.h"
#include "types.h"
#include "debug.h"
#include "iv_adc.h"

#define BUFLEN 1024
static char buf[BUFLEN];

char *
readline(void)
{
  int i, c, r;
  //  char buf[40];
  i=0;
  uint16_t data;
  while(1){
    //check for incoming USB data
    //first check data port
    if (udi_cdc_multi_is_rx_ready(USB_DATA_PORT)) {
      c = udi_cdc_multi_getc(USB_DATA_PORT);
      if (c < 0) {
	sprintf(buf,"read error: %d\n", c);
	print(buf);
	return NULL;
      } else if (c >= ' ' && i < BUFLEN-1) {
	buf[i++] = c;
      } else if (c == '\n' || c == '\r') {
	buf[i] = 0;
	return buf;
      }
    }
  }
}
