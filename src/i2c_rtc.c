#include <asf.h>
#include "debug.h"
#include "i2c_rtc.h"


//initialize the i2c bus
int i2c_rtc_init(void){
  twi_options_t opt;
  uint32_t r;
  char buf [30];

  //enable peripherial mode on TWI lines
  gpio_configure_pin(PIO_PA3_IDX,(PIO_PERIPH_A | PIO_DEFAULT));
  gpio_configure_pin(PIO_PA4_IDX,(PIO_PERIPH_A | PIO_DEFAULT));

  //enable TWI clock
  pmc_enable_periph_clk(RTC_ID_TWI);
  //setup up TWI options
  opt.master_clk = sysclk_get_cpu_hz();
  opt.speed = 400000; //400kHz
  opt.smbus = 0x0;
  opt.chip = 0x0;
  if((r=twi_master_init(RTC_BASE_TWI, &opt)) != TWI_SUCCESS){
    sprintf(buf,"error setting up I2C for RTC: %d\n",r);
    print(buf);
    return -1;
  }
  return 0;
}
//returns unix timestamp 
uint32_t i2c_rtc_get_time(void){
  uint32_t r;
  char buf [30];
  twi_packet_t packet_rx;
  uint8_t rx_data [8];
  int h,l;
  packet_rx.chip = RTC_ADDRESS;
  packet_rx.addr[0] = 0x0;
  packet_rx.addr_length = 1;
  packet_rx.buffer = rx_data;
  packet_rx.length = 7;
  if((r=twi_master_read(RTC_BASE_TWI, &packet_rx)) != TWI_SUCCESS){
    sprintf(buf,"error reading RTC: %d\n",r);
    print(buf);
    return 0;
  }
  //convert from BCD to decimal
  h = (rx_data[0]&0xF0)>>4;
  l = (rx_data[0]&0x0F);
  return h*10+l;
}
//set RTC with unix timestamp
int i2c_rtc_set_time(uint32_t val){
  //transmit the full RTC date, 7 registers
  // SC | MN | HR | DW | DT | MO | YR
  uint32_t r;
  char buf [30];
  uint8_t tx_data [8];
  twi_packet_t packet_tx;
  tx_data[0] = (uint8_t)val; //sec
  tx_data[1] = 1; //min
  tx_data[2] = 1; //hr
  tx_data[3] = 2;
  tx_data[4] = 3;
  tx_data[5] = 4;
  tx_data[6] = 15;

  packet_tx.chip = RTC_ADDRESS;
  packet_tx.addr[0] = 0x0;
  packet_tx.addr_length = 1;
  packet_tx.buffer = tx_data;
  packet_tx.length = 7;
  if((r=twi_master_write(RTC_BASE_TWI, &packet_tx)) != TWI_SUCCESS){
    sprintf(buf,"error setting RTC: %d\n",r);
    print(buf);
    return -1;
  }
  return 0;
}
