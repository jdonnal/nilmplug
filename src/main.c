#include <asf.h>
#include <string.h>
#include "debug.h"
#include "usb.h"
#include "rtc.h"
#include "wifi.h"
#include "monitor.h"
#include "wemo_fs.h"
#include "rgb_led.h"

#include <stdio.h>

//! Pointer to the external low level write function.
extern int (*ptr_put)(void volatile*, char);


void io_init(void);
void relay_on(void);
void relay_off(void);

int main(void) {

  ptr_put = (int (*)(void volatile*,char))&dbgputc;
  setbuf(stdout, NULL);

  // Switch over to the crystal oscillator
  sysclk_init();
  // Disable the built-in watchdog timer
  wdt_disable(WDT);
  //initialize peripherals
  io_init();
  rgb_led_init();
  rgb_led_set(242,222,68);
  cpu_irq_enable();
  usb_init();
  i2c_rtc_init();
  char buf[100];
  rtc_get_time_str(buf);
  printf(buf); printf("\n");
  //  i2c_rtc_set_time(1,2,3,4,5,6,7);
  wemo_fs_init(); //file system (config and logging)
  if(wifi_init()!=0){
    rgb_led_set(255,0,0);
    while(1);
  }
  server_init(); 
  rgb_led_set(0,125,30);
  printf("entering monitor\n");
  monitor();
  printf("uh oh... out of monitor\n");
  while(1){
    //should never get here
  };
}


void io_init(void){
  //Relay
  pmc_enable_periph_clk(ID_PIOA);
  gpio_configure_pin(RELAY_PIN, PIO_OUTPUT_0);

  //SD Card pins
  gpio_configure_pin(PIO_PA1_IDX, PIO_OUTPUT_0);
  gpio_configure_pin(PIN_HSMCI_MCCDA_GPIO, PIN_HSMCI_MCCDA_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCCK_GPIO, PIN_HSMCI_MCCK_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA0_GPIO, PIN_HSMCI_MCDA0_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA1_GPIO, PIN_HSMCI_MCDA1_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA2_GPIO, PIN_HSMCI_MCDA2_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA3_GPIO, PIN_HSMCI_MCDA3_FLAGS);
}
