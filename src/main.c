#include <asf.h>
#include <string.h>
#include "debug.h"
#include "usb.h"
#include "rtc.h"
#include "monitor.h"
#include <stdio.h>

//! Pointer to the external low level write function.
extern int (*ptr_put)(void volatile*, char);

int main(void) {
  Ctrl_status status;
  FRESULT res;
  FATFS fs;
  FIL file_object;
  ptr_put = (int (*)(void volatile*,char))&usb_putc;
  setbuf(stdout, NULL);

  // Switch over to the crystal oscillator
  sysclk_init();
  //delay_0init(get_cpu_hz());
  // Disable the built-in watchdog timer
  wdt_disable(WDT);
  //initialize peripherals
  usb_init();
  sd_mmc_init();
  i2c_rtc_init();
  //make sure the SD card is available
  //  gpio_configure_pin(PIO_PB14_IDX, PIO_OUTPUT_0);
	
  printf("asdf");
  do {
    status = sd_mmc_test_unit_ready(0);
    if (CTRL_FAIL == status) {
      printf("Card install FAIL\n");
      printf("Please unplug and re-plug the card.\n");
      while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
      }
    }
  } while (CTRL_GOOD != status);
  //mount the FS
  print("Mount disk (f_mount)...\r\n");
  memset(&fs, 0, sizeof(FATFS));
  res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
  if (FR_INVALID_DRIVE == res) {
    printf("[FAIL] res %d\r\n",res);
  }
  printf("[OK]\r\n");
  
  char buf [30];
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
