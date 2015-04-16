#include <asf.h>
#include <string.h>
#include "debug.h"
#include "usb.h"
#include "rtc.h"
#include "wifi.h"
#include "monitor.h"
#include <stdio.h>

//! Pointer to the external low level write function.
extern int (*ptr_put)(void volatile*, char);

//initialize IO pins
void io_init(void);

int main(void) {
  Ctrl_status status;
  FRESULT res;
  FATFS fs;
  FIL file_object;
  char test_file_name[] = "0:sd_mmc_test.txt";

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
  //  sd_mmc_init();
  wifi_init();
  //install the SD Card
  while(1);
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
  printf("Mount disk (f_mount)...\r\n");
  memset(&fs, 0, sizeof(FATFS));
  res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
  if (FR_INVALID_DRIVE == res) {
    printf("[FAIL] res %d\r\n",res);
  }
  printf("[OK]\r\n");

  printf("Create a file (f_open)...\r\n");
  test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
  res = f_open(&file_object,
	       (char const *)test_file_name,
	       FA_CREATE_ALWAYS | FA_WRITE);
  if (res != FR_OK) {
    printf("[FAIL] res %d\r\n", res);
    goto main_end_of_test;
  }
  printf("[OK]\r\n");
  
  printf("Write to test file (f_puts)...\r\n");
  if (0 == f_puts("Test SD/MMC stack\n", &file_object)) {
    f_close(&file_object);
    printf("[FAIL]\r\n");
    goto main_end_of_test;
  }
  printf("[OK]\r\n");
  f_close(&file_object);
  printf("Test is successful.\n\r");
 main_end_of_test:
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
