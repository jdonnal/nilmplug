#include <asf.h>
#include <stdio.h>
#include "string.h"
#include "wifi.h"
#include "server.h"
#include "wemo_fs.h" //for wemo_config

//** Globals for wifi_send_cmd **
uint8_t rx_buf [RESP_BUF_SIZE];
uint32_t rx_buf_idx = 0;
int data_tx_status = TX_IDLE;

bool rx_wait = false; //we are waiting for response
char rx_complete_str[30]; //expected response of command to ESP8266
bool rx_complete = false; //flag set by UART int when rx_complete_str matches
bool data_tx = false; //we are transmitting data

//local index into server buffer
uint32_t server_buf_idx = 0;

int wifi_send_cmd(const char* cmd, const char* resp_complete, char* resp, 
		  uint32_t maxlen, int timeout);
int wifi_send_data(int ch, const char* data);

int wifi_init(void){
  uint32_t BUF_SIZE = 300;
  int num_tries = 0;
  uint32_t idx; //tmp variable for string indexing
  char buf [BUF_SIZE];
  char tx_buf [BUF_SIZE];
  //set up the UART
  static usart_serial_options_t usart_options = {
    .baudrate = WIFI_UART_BAUDRATE,
    .charlength = WIFI_UART_CHAR_LENGTH,
    .paritytype = WIFI_UART_PARITY,
    .stopbits = WIFI_UART_STOP_BITS
  };
  gpio_configure_pin(PIO_PA9_IDX, (PIO_PERIPH_A | PIO_DEFAULT));
  gpio_configure_pin(PIO_PA10_IDX, (PIO_PERIPH_A | PIO_DEFAULT));
  pmc_enable_periph_clk(ID_WIFI_UART);
  sysclk_enable_peripheral_clock(ID_WIFI_UART);
  usart_serial_init(WIFI_UART,&usart_options);
  //flush any existing data
  uint8_t tmp;
  while(usart_serial_is_rx_ready(WIFI_UART)){
    usart_serial_getchar(WIFI_UART,&tmp);
  }
  // Trigger from timer 0
  pmc_enable_periph_clk(ID_TC0);
  tc_init(TC0, 0,                        // channel 0
	  TC_CMR_TCCLKS_TIMER_CLOCK5     // source clock (CLOCK5 = Slow Clock)
	  | TC_CMR_CPCTRG                // up mode with automatic reset on RC match
	  | TC_CMR_WAVE                  // waveform mode
	  | TC_CMR_ACPA_CLEAR            // RA compare effect: clear
	  | TC_CMR_ACPC_SET              // RC compare effect: set
	  );
  TC0->TC_CHANNEL[0].TC_RA = 0;  // doesn't matter
  TC0->TC_CHANNEL[0].TC_RC = 64000;  // sets frequency: 32kHz/32000 = 1 Hz
  NVIC_ClearPendingIRQ(TC0_IRQn);
  NVIC_SetPriority(TC0_IRQn,0);
  NVIC_EnableIRQ(TC0_IRQn);
  tc_enable_interrupt(TC0, 0, TC_IER_CPCS);
  //reset the module
  if(wifi_send_cmd("AT+RST","ready",buf,BUF_SIZE,1)==0){
    printf("Error reseting ESP8266\n");
    return 0;
  }
  delay_ms(500);
  //clear out old access point info
  //  wifi_send_cmd("AT+CWJAP=\"\",\"\"",buf,BUF_SIZE,4);
  //printf(buf);
  //see if we are using WiFi
  if(wemo_config.standalone)
    return 0; 
  //set to mode STA
  if(wifi_send_cmd("AT+CWMODE=1","no change",buf,BUF_SIZE,1)==0){
    printf("Error setting ESP8266 mode\n");
    return 0;
  }
  delay_ms(500);
  while(num_tries<MAX_TRIES){
    //try to join the specified network  
    sprintf(tx_buf,"AT+CWJAP=\"%s\",\"%s\"",wemo_config.wifi_ssid,wemo_config.wifi_pwd);
    if(wifi_send_cmd(tx_buf,"OK",buf,BUF_SIZE,5)==0){
      printf("no response to CWJAP\n");
      num_tries++;
      continue;
    }
    if(strstr(buf,"ERROR")==buf){
      printf("error %d/%d joining network\n",num_tries,MAX_TRIES);
      num_tries++;
      continue;
    } else {
      break;
    }
  }
  if(num_tries>=MAX_TRIES){
  //log the failure
    sprintf(tx_buf,"Could not join [%s]",wemo_config.wifi_ssid);
    wemo_log(tx_buf);
    printf("error joinining [%s]\n",wemo_config.wifi_ssid);
    return -1;
  }
  //see if we have an IP address
  wifi_send_cmd("AT+CIFSR","OK",buf,BUF_SIZE,2);
  if(strstr(buf,"ERROR")==buf){
    printf("error getting IP address\n");
    return 0;
  }
  //save the IP to our config
  memset(wemo_config.wemo_ip,0,30);
  idx = (uint32_t)strstr(buf,"\r\n")-(uint32_t)buf;
  memcpy(wemo_config.wemo_ip,buf,idx);
  //set the mode to multiple connections
  wifi_send_cmd("AT+CIPMUX=1","OK",buf,BUF_SIZE,2);
  //start a server on port 1336
  wifi_send_cmd("AT+CIPSERVER=1,1336","OK",buf,BUF_SIZE,2);
  //log the event
  sprintf(tx_buf,"Joined [%s] with IP [%s]",wemo_config.wifi_ssid,wemo_config.wemo_ip);
  printf("%s\n",tx_buf);
  wemo_log(tx_buf);
  return 0;
}

int wifi_transmit(char *url, int port, char *data){
  char cmd[100];
  char buf[100];
  //sometimes the port stays open, so check for both
  //conditions
  char *success_str = "OK";
  char *connected_str = "ALREAY CONNECT";
  //open a TCP connection on channel 4
  sprintf(cmd,"AT+CIPSTART=4,\"TCP\",\"%s\",%d",
	  url,port);
  wifi_send_cmd(cmd,"Linked",buf,100,2);
  //if we are still connected, close and reopen socket
  if(strstr(buf,connected_str)==buf){
    wifi_send_cmd("AT+CIPCLOSE=4","Unlink",buf,100,1);
    //now try again
    wifi_send_cmd(cmd,"Linked",buf,100,2);
  }
  //check for successful link
  if(strstr(buf,success_str)!=buf){
    printf("error, setting up link\n");
    return -1;
  }
  //send the data
  if(wifi_send_data(4,data)!=0){
    printf("error transmitting data: %d\n",data_tx_status);
    return -1;
  }
  //close the connection
  wifi_send_cmd("AT+CIPCLOSE=4","Unlink",buf,100,1);

  return 0; //success!
}

//****OBSOLETE****
int wifi_server_start(void){
  uint32_t BUF_SIZE = 300;
  char buf [BUF_SIZE];
  wifi_send_cmd("AT+CIPMUX=1","OK",buf,BUF_SIZE,2);
  wifi_send_cmd("AT+CIPSERVER=1,1336","OK",buf,BUF_SIZE,2);
  return 0;
};

//Process the server_buf string
void wifi_process_data(void){
  //if we are linked, look for incoming data
  gpio_toggle_pin(PIO_PA13_IDX);

  memset(server_buf,0x0,SERVER_BUF_SIZE);
  server_buf_idx=0;
}

int wifi_send_data(int ch, const char* data){
  char cmd[100];
  int timeout = 2; //wait 2 seconds to transmit the data
  data_tx = true;
  sprintf(cmd,"AT+CIPSEND=4,%d\r\n",strlen(data));
  data_tx_status=TX_PENDING;
  rx_wait=true;
  rx_buf_idx = 0;
  memset(rx_buf,0x0,RESP_BUF_SIZE);
  memset(server_buf,0x0,SERVER_BUF_SIZE); //to make debugging easier
  server_buf_idx=0;
  usart_serial_write_packet(WIFI_UART,(uint8_t*)cmd,strlen(cmd));
  usart_serial_write_packet(WIFI_UART,(uint8_t*)data,strlen(data));
  //now wait for the data to be sent
  while(timeout>0){
    //start the timer
    tc_start(TC0, 0);	  
    //when timer expires, return what we have in the buffer
    rx_wait=true; //reset the wait flag
    while(rx_wait && data_tx_status!=TX_SUCCESS);
    tc_stop(TC0,0);
    if(data_tx_status==TX_SUCCESS){
      delay_ms(500); //wait 0.5 sec to discard returned data (hack)
      data_tx_status=TX_IDLE;
      rx_wait = false;
      return 0;
    }
    timeout--;
  }
  //an error occured, see if it is due to a module reset
  if(strstr((char*)rx_buf,"\r\nready\r\n")==(char*)rx_buf){
    //module reset itself!!! 
    printf("detected module reset\n");
  }
  data_tx_status=TX_ERROR;
  return -1;
}

int wifi_send_cmd(const char* cmd, const char* resp_complete,
		  char* resp, uint32_t maxlen, int timeout){
  rx_buf_idx = 0;
  uint32_t rx_start, rx_end;
  //clear out the response buffer
  memset(resp,0x0,maxlen);
  memset(rx_buf,0x0,RESP_BUF_SIZE);
  //setup the rx_complete buffer so we know when the command is finished
  memcpy(rx_complete_str,resp_complete,strlen(resp_complete));
  rx_complete_str[strlen(resp_complete)]='\r';
  rx_complete_str[strlen(resp_complete)+1]='\n';
  rx_complete_str[strlen(resp_complete)+2]='\0';
  //enable RX interrupts
  usart_enable_interrupt(WIFI_UART, US_IER_RXRDY);
  NVIC_EnableIRQ(WIFI_UART_IRQ);
  //write the command
  rx_wait=true; //we want this data returned in rx_buf
  rx_complete =false; //reset the early complete flag
  usart_serial_write_packet(WIFI_UART,(uint8_t*)cmd,strlen(cmd));
  //terminate the command
  usart_serial_putchar(WIFI_UART,'\r');
  usart_serial_putchar(WIFI_UART,'\n');
  //wait for [timeout] seconds
  while(timeout>0){
    //start the timer
    tc_start(TC0, 0);	  
    //when timer expires, return what we have in the buffer
    rx_wait=true; //reset the wait flag
    while(rx_wait);
    tc_stop(TC0,0);
    if(rx_complete) //if the uart interrupt signals rx is complete
      break;
    timeout--;
  }
  //now null terminate the response
  rx_buf[rx_buf_idx]=0x0;
  //remove any ECHO
  if(strstr((char*)rx_buf,cmd)!=(char*)rx_buf){
    printf("bad echo\n");
    return 0;
  } 
  rx_start = strlen(cmd);
  //remove leading whitespace
  while(rx_buf[rx_start]=='\r'|| rx_buf[rx_start]=='\n')
    rx_start++;
  //remove trailing whitespace
  rx_end = strlen((char*)rx_buf)-1;
  while(rx_buf[rx_end]=='\r'|| rx_buf[rx_end]=='\n')
    rx_end--;
  //make sure we have a response
  if(rx_end<=rx_start){
    printf("no response by timeout\n");
    return 0;
  }
  //copy the data to the response buffer
  if((rx_end-rx_start+1)>maxlen){
    memcpy(resp,&rx_buf[rx_start],maxlen-1);
    resp[maxlen-1]=0x0;
    printf((char*)rx_buf);
    printf("truncated output!\n");
  } else{
    memcpy(resp,&rx_buf[rx_start],rx_end-rx_start+1);
    //null terminate the response buffer
    resp[rx_end-rx_start+1]=0x0;
  }
  return rx_end-rx_start;
}


ISR(TC0_Handler)
{
  rx_wait = false;
  //clear the interrupt so we don't get stuck here
  tc_get_status(TC0,0);
}

ISR(UART0_Handler)
{
  uint8_t tmp, i;
  usart_serial_getchar(WIFI_UART,&tmp);
  //check whether this is a command response or 
  //new data from the web (unsollicted response)
  if(rx_wait && data_tx_status!=TX_PENDING){
    if(rx_buf_idx>=RESP_BUF_SIZE){
      printf("error!\n");
      return; //ERROR!!!!!
    }
    rx_buf[rx_buf_idx++]=(char)tmp;
    //check for completion_str to indicate completion of command
    //this is just a speed up, we still timeout regardless
    //of whether we find this string 
    if(rx_buf_idx>4){
      if(strstr((char*)rx_buf,rx_complete_str)==
	 (char*)&rx_buf[rx_buf_idx-strlen(rx_complete_str)]){
	rx_complete = true; //early completion!
	rx_wait = false; 
      }
    }
  } else if(rx_wait && data_tx_status==TX_PENDING){
    //we are transmitting, no need to capture response,
    //just wait for SEND OK\r\n, use a 9 char circular buffer
    if(rx_buf_idx>8){
      for(i=0;i<8;i++)
	rx_buf[i]=rx_buf[i+1];
      rx_buf[8]=tmp;
      if(strstr((char*)rx_buf,"SEND OK\r\n")==(char*)rx_buf){
	data_tx_status=TX_SUCCESS;
      }
    } else{
      rx_buf[rx_buf_idx++]=tmp;
    }
  }else { //this is unsollicted data
    if(server_buf_idx>=SERVER_BUF_SIZE){
      printf("error!\n");
      return; //ERROR!!!!
    }
    server_buf[server_buf_idx++]=(char)tmp;
    //if we have a full line, send it to the server process
    if(server_buf_idx>=6){
      //check for link 
      if((strstr(server_buf,"Link\r\n")==server_buf) ||
	 (strstr(server_buf,"Linked\r\n")==server_buf)){
	server_link();
	server_buf_idx=0;
	return;
      }
      //check for unlink
      if(strstr(server_buf,"Unlink\r\n")==server_buf){
	server_unlink();
	server_buf_idx=0;
	return;
      }
      //check for data
      if(strstr(&server_buf[server_buf_idx-4],"OK\r\n")==
	 &server_buf[server_buf_idx-4]){
	server_process_data();
	server_buf_idx=0;
	return;
      }
    }
    //otherwise keep collecting data
  }
}
