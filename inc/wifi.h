#ifndef __WIFI_H__
#define __WIFI_H__

#define RESP_BUF_SIZE 400
#define MAX_TRIES 3 //number of times to try joining a network

#define TX_IDLE     2
#define TX_PENDING  1
#define TX_SUCCESS  0
#define TX_ERROR   -1
#define TX_TIMEOUT -2
#define TX_BAD_DEST_IP -3

int wifi_init(void);

int wifi_server_start(void);
int wifi_transmit(char *url, int port, char *data);
int wifi_send_cmd(const char* cmd, const char* resp_complete, char* resp, 
		  uint32_t maxlen, int timeout);

int wifi_send_data(int ch, const char* data);
#endif
