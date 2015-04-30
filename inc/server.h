#ifndef __SERVER_H__
#define __SERVER_H__

#define SERVER_BUF_SIZE 1000

char* server_buf;
int server_buf_len;

void server_init(void);

void server_process_data(void);
void server_link(void);
void server_unlink(void);

#endif
