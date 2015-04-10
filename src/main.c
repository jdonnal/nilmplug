#include <asf.h>
#include "debug.h"
#include "usb.h"
#include "i2c_rtc.h"
#include "monitor.h"




int main(void) {

  // Switch over to the crystal oscillator
  sysclk_init();
  //delay_init(get_cpu_hz());
  // Disable the built-in watchdog timer
  wdt_disable(WDT);
  //initialize peripherals
  usb_init();
  i2c_rtc_init();
  char buf [30];
  //  i2c_rtc_set_time(32);
  uint32_t val;
  while(1){
    delay_s(1);
    val = i2c_rtc_get_time();
    sprintf(buf,"seconds: %d\n",(uint8_t)val);
    print(buf);
  }
  // LEDs
  pmc_enable_periph_clk(ID_PIOA);
  
  gpio_configure_pin(PIO_PA17_IDX, PIO_OUTPUT_1);
  gpio_configure_pin(PIO_PA18_IDX, PIO_OUTPUT_1);
  gpio_configure_pin(PIO_PA0_IDX, PIO_OUTPUT_1);
  gpio_set_pin_high(PIO_PA0_IDX);
  gpio_set_pin_low(PIO_PA17_IDX);
  gpio_set_pin_low(PIO_PA18_IDX);
  
  print("entering monitor\n");
  monitor();
  print("uh oh... out of monitor\n");
  while(1){
    //should never get here
  };
}
