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

  ptr_put = (int (*)(void volatile*,char))&core_putc;
  setbuf(stdout, NULL);

  // Switch over to the crystal oscillator
  sysclk_init();
  //***********DISABLE WDT*********
  wdt_disable(WDT);
  //initialize GPIO
  io_init();
  rgb_led_init();
  rgb_led_set(242,222,68);
  usb_init();
  i2c_rtc_init();
  fs_init(); //file system (config and logging)
  wemo_init();  //interacting with WEMO
  //initialize system tick with PWM
  pmc_enable_periph_clk(ID_PWM);
  pwm_channel_disable(PWM, 0);
  pwm_clock_t clock_setting = {
    .ul_clka = 1000, //1 kHz
    .ul_clkb = 0, //not used
    .ul_mck = sysclk_get_cpu_hz()
  };
  pwm_init(PWM, &clock_setting);
  //               turn on channel 0
  pwm_channel_t channel = {
    .channel = 0,
    .ul_duty = 0,
    .ul_period = 5000, //sample every 5 seconds
    .ul_prescaler = PWM_CMR_CPRE_CLKA,
    .polarity = PWM_HIGH,
  };
  pwm_channel_init(PWM, &channel);
  //                enable interrupts on overflow
  pwm_channel_enable_interrupt(PWM,0,0);
  pwm_channel_enable(PWM,0);
  NVIC_SetPriority(PWM_IRQn,3); //lowest priority
  NVIC_EnableIRQ(PWM_IRQn);
  /*    ----NOT USING WDT-----
  // Configure WDT to trigger an interrupt (or reset). 
  uint32_t wdt_mode = 
    WDT_MR_WDRSTEN   |  // WDT fault triggers resets 
    WDT_MR_WDRPROC   |  // WDT fault resets processor only. 
    WDT_MR_WDDBGHLT  |  // WDT stops in debug state. 
    WDT_MR_WDIDLEHLT;   // WDT stops in idle state. 
  uint32_t timeout_value = wdt_get_timeout_value(WDT_PERIOD * 1000,
					32768);
  // Initialize WDT with the given parameters. 
  wdt_init(WDT, wdt_mode, timeout_value, timeout_value);
  //check if previous reset was due to watchdog
  uint32_t info = rstc_get_status(RSTC);
  if(info & RSTC_WATCHDOG_RESET){
    printf("watchdog reset!\n");
    core_log("watchdog reset");
  }
  -----------------     */
  //ready to go! enable interrupts
  cpu_irq_enable();

  printf("entering monitor\n");
  monitor();
  printf("uh oh... out of monitor\n");
  while(1){
    //should never get here
  };
}


void io_init(void){
  pmc_enable_periph_clk(ID_PIOA);
  pmc_enable_periph_clk(ID_PIOB);
  
  //Relay
  //restore relay state from backup register
  if(gpbr_read(GPBR_RELAY_STATE)==1){
    printf("relay starting on\n");
    gpio_configure_pin(RELAY_PIN, PIO_OUTPUT_1);
  }
  else{
    printf("relay starting off\n");
    gpio_configure_pin(RELAY_PIN, PIO_OUTPUT_0);
  }

  //VBUS
  gpio_configure_pin(VBUS_PIN, PIO_INPUT);
  //Button
  gpio_configure_pin(BUTTON_PIN, PIO_INPUT);
  
  //SD Card pins
  gpio_configure_pin(PIN_HSMCI_MCCDA_GPIO, PIN_HSMCI_MCCDA_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCCK_GPIO, PIN_HSMCI_MCCK_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA0_GPIO, PIN_HSMCI_MCDA0_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA1_GPIO, PIN_HSMCI_MCDA1_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA2_GPIO, PIN_HSMCI_MCDA2_FLAGS);
  gpio_configure_pin(PIN_HSMCI_MCDA3_GPIO, PIN_HSMCI_MCDA3_FLAGS);
}
