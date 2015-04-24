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

//initialize IO pins
#define RELAY_PIN PIO_PA13_IDX

void io_init(void);
void relay_on(void);
void relay_off(void);

int main(void) {

  char buf[50];

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
  usb_init();
  i2c_rtc_init();
  wemo_fs_init(); //file system (config and logging)
  if(wifi_init()!=0){
    rgb_led_set(255,0,0);
    while(1);
  }
  rgb_led_set(0,125,30);
  //now we are ready!
  //start a server and any incoming data toggles the relay
  wifi_server_start();
  printf("entering monitor\n");
  monitor();
  printf("uh oh... out of monitor\n");
  while(1){
    //should never get here
  };
}


void relay_on(void){
  gpio_set_pin_high(RELAY_PIN);
}

void relay_off(void){
  gpio_set_pin_low(RELAY_PIN);
}

void relay_toggle(void){
  gpio_toggle_pin(RELAY_PIN);
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
