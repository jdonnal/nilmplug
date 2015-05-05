#ifndef __SERVER_H__
#define __SERVER_H__

#define SERVER_BUF_SIZE 300
#define PKT_SIZE 10
typedef struct power_pkt_struct {
  uint8_t status;             //struct valid flag
  char timestamp[40];         //YYMMDD HH:MM:ss
  int32_t vrms[PKT_SIZE];     //RMS voltage
  int32_t irms[PKT_SIZE];     //RMS current
  int32_t watts[PKT_SIZE];    //watts
  int32_t pavg[PKT_SIZE];     //Average power (30s window)
  int32_t freq[PKT_SIZE];     //Line frequency
  int32_t pf[PKT_SIZE];       //Power factor
  int32_t kwh[PKT_SIZE];      //kWh since turn on
} power_pkt;


typedef struct power_sample_struct {
  uint8_t valid;          //struct valid flag
  int32_t vrms;           //RMS voltage
  int32_t irms;           //RMS current
  int32_t watts;          //watts
  int32_t pavg;           //Average power (30s window)
  int32_t freq;           //Line frequency
  int32_t pf;             //Power factor
  int32_t kwh;            //kWh since turn on
} power_sample;

#define POWER_PKT_EMPTY      0  //all fields are empty
#define POWER_PKT_FILLING    1  //fields are being filled
#define POWER_PKT_READY      2  //all fields full, ready to send
#define POWER_PKT_TX_IN_PROG 3  //transmission in progress
#define POWER_PKT_TX_FAIL    4  //failed transmission


extern power_sample wemo_sample;

char* server_buf;
int server_buf_len;

void server_init(void);
void server_read_power(void);
void server_process_data(void);
void server_link(void);
void server_unlink(void);

#endif
