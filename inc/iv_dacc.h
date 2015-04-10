#ifndef __DACC_H__
#define __DACC_H__


//two DACC channels (current and voltage)
#define DACC_CH_I 0
#define DACC_CH_V 1

//enable debugging
//#define DACC_DBG

void dacc_init(void);

void dacc_write(uint8_t ch, uint16_t value);


#endif
