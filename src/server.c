#include <asf.h>
#include "string.h"
#include "server.h"
#include "monitor.h"


void server_init(void){
  //allocate memory for the server buffer
  server_buf = (char*)malloc(SERVER_BUF_SIZE);
  server_buf_len=SERVER_BUF_SIZE;
};

void server_process_data(void){
  char data[100];
  int chan_num, data_size;
  //match against the data
  sscanf(server_buf,"\r\n+IPD,%d,%d:%s", &chan_num, &data_size, data);
  printf("Got [%d] bytes on channel [%d]: %s\n",
	 data_size, chan_num, data);
  if(strstr(data,"relay_on")==data){
    mon_relay_on(0,NULL);
  }
  if(strstr(data,"relay_off")==data){
    mon_relay_off(0,NULL);
  }
  else{
    printf("unknown command: %s\n",data);
  }
  //clear the server buffer
  memset(server_buf,0x0,server_buf_len);
}

void server_link(void){
  printf("linked!\n");
  //clear the server buffer
  memset(server_buf,0x0,server_buf_len);
}

void server_unlink(void){
  printf("unlinked\n");
  //clear the server buffer
  memset(server_buf,0x0,server_buf_len);
}
