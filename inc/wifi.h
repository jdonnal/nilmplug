#ifndef __WIFI_H__
#define __WIFI_H__

#define RESP_BUF_SIZE 400
#define SERVER_BUF_SIZE 300
#define MAX_TRIES 3 //number of times to try joining a network

int wifi_init(void);

int wifi_server_start(void);
int wifi_transmit(char *url, int port, char *data);

#endif
