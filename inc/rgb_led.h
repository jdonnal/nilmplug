#ifndef __RGB_LED_H__
#define __RGB_LED_H__

#define RGB_LED_PIN    (PIO_PA17_IDX)
#define RGB_SET_HIGH   (PIOA->PIO_SODR = 1<<17)
#define RGB_SET_LOW    (PIOA->PIO_CODR = 1<<17)
void rgb_led_init(void);
void rgb_led_set(uint8_t r, uint8_t g, uint8_t b);

#endif
