#ifndef __IV_ADC_H__
#define __IV_ADC_H__


#define ADC_CH_V ADC_CHANNEL_4
#define ADC_CH_I1 ADC_CHANNEL_6
#define ADC_CH_I2 ADC_CHANNEL_3
#define ADC_CH_I3 ADC_CHANNEL_2

#define LED_ADC_INT PIO_PA7_IDX
#define LED_TX_FULL PIO_PA10_IDX

/* Tracking Time*/
#define TRACKING_TIME         1
/* Transfer Period */
#define TRANSFER_PERIOD       1

/* FIFO data buffer for USB */
#define FIFO_BUFFER_LENGTH 128
union buffer_element {
  uint8_t byte;
  uint16_t halfword;
  uint32_t word;
};

extern fifo_desc_t adc_fifo_desc;
extern union buffer_element adc_fifo[];


void iv_adc_init(void);
void iv_adc_start(void);
void iv_adc_stop(void);

#endif
