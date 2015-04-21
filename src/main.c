#include <asf.h>
#include <string.h>
#include "debug.h"
#include "usb.h"
#include "rtc.h"
#include "wifi.h"
#include "monitor.h"
#include "wemo_fs.h"
#include <stdio.h>

//! Pointer to the external low level write function.
extern int (*ptr_put)(void volatile*, char);

//initialize IO pins
void io_init(void);

int main(void) {

  ptr_put = (int (*)(void volatile*,char))&dbgputc;
  setbuf(stdout, NULL);

  // Switch over to the crystal oscillator
  sysclk_init();
  // Disable the built-in watchdog timer
  wdt_disable(WDT);
  //initialize peripherals
  io_init();
  usb_init();
  i2c_rtc_init();
  wemo_fs_init(); //file system (config and logging)
  wifi_init();
  printf("entering monitor\n");
  monitor();
  printf("uh oh... out of monitor\n");
  while(1){
    //should never get here
  };
}


void io_init(void){
  //LED's
  pmc_enable_periph_clk(ID_PIOA);
  gpio_configure_pin(PIO_PA17_IDX, PIO_OUTPUT_1);
  gpio_configure_pin(PIO_PA18_IDX, PIO_OUTPUT_1);
  gpio_configure_pin(PIO_PA0_IDX, PIO_OUTPUT_1);
  gpio_configure_pin(PIO_PB14_IDX, PIO_OUTPUT_1);
  gpio_set_pin_high(PIO_PA0_IDX);
  gpio_set_pin_low(PIO_PA17_IDX);
  gpio_set_pin_low(PIO_PA18_IDX);

  //SD Card pins
  gpio_configure_pin(PIO_PA1_IDX, PIO_OUTPUT_0);
  gpio_configure_pin(PIN_HSMCI_MCCDA_GPIO, PIN_HSMCI_MCCDA_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCCK_GPIO, PIN_HSMCI_MCCK_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA0_GPIO, PIN_HSMCI_MCDA0_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA1_GPIO, PIN_HSMCI_MCDA1_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA2_GPIO, PIN_HSMCI_MCDA2_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA3_GPIO, PIN_HSMCI_MCDA3_FLAGS);
}
