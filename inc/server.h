#ifndef __SERVER_H__
#define __SERVER_H__

#define SERVER_BUF_SIZE 300

typedef struct power_pkt_struct {
  uint8_t status;             //struct valid flag
  char timestamp[40];         //YYMMDD HH:MM:ss
  int32_t vrms[10];           //RMS voltage
  int32_t irms[10];           //RMS current
  int32_t watts[10];          //watts
  int32_t pavg[10];           //Average power (30s window)
  int32_t freq[10];           //Line frequency
  int32_t pf[10];             //Power factor
  int32_t kwh[10];            //kWh since turn on
} power_pkt;


typedef struct power_sample_struct {
  uint8_t valid;             //struct valid flag
  char timestamp[40];         //YYMMDD HH:MM:ss
  int32_t vrms[10];           //RMS voltage
  int32_t irms[10];           //RMS current
  int32_t watts[10];          //watts
  int32_t pavg[10];           //Average power (30s window)
  int32_t freq[10];           //Line frequency
  int32_t pf[10];             //Power factor
  int32_t kwh[10];            //kWh since turn on
} power_sample;

#define POWER_PKT_EMPTY      0  //all fields are empty
#define POWER_PKT_FILLING    1  //fields are being filled
#define POWER_PKT_READY      2  //all fields full, ready to send
#define POWER_PKT_TX_IN_PROG 3  //transmission in progress
#define POWER_PKT_TX_FAIL    4  //failed transmission


extern power_pkt wemo_power;

char* server_buf;
int server_buf_len;

void server_init(void);
void server_read_power(void);
void server_process_data(void);
void server_link(void);
void server_unlink(void);

#endif
