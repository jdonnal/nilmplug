#ifndef __I2C_RTC_H__
#define __I2C_RTC_H__

#define RTC_ID_TWI ID_TWI0
#define RTC_BASE_TWI TWI0
#define RTC_ADDRESS 0x68

//initialize the i2c bus
int i2c_rtc_init(void);
//returns unix timestamp 
uint32_t i2c_rtc_get_time(void);
//set RTC with unix timestamp
int i2c_rtc_set_time(uint32_t val);
#endif
