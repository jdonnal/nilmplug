#ifndef __SERVER_H__
#define __SERVER_H__

#define SERVER_BUF_SIZE 300

typedef struct power_pkt_struct {
  uint8_t valid;          //struct valid flag
  int32_t vrms;           //RMS voltage
  int32_t irms;           //RMS current
  int32_t watts;          //watts
  int32_t pavg;           //Average power (30s window)
  int32_t freq;           //Line frequency
  int32_t pf;             //Power factor
  int32_t kwh;            //kWh since turn on
} power_pkt;


extern power_pkt wemo_power;

char* server_buf;
int server_buf_len;

void server_init(void);
void server_read_power(void);
void server_process_data(void);
void server_link(void);
void server_unlink(void);

#endif
