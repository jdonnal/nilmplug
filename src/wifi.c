#include <asf.h>
#include <stdio.h>
#include "string.h"
#include "wifi.h"

void write_buffer(char* str);

int wifi_send_cmd(const char* cmd, char* resp, uint32_t maxlen, int timeout);

void wifi_init(void){
  uint32_t BUF_SIZE = 100;
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
  cpu_irq_enable();
  NVIC_EnableIRQ(TC0_IRQn);
  tc_enable_interrupt(TC0, 0, TC_IER_CPCS);
  //reset the module
  printf("AT+RST:");
  wifi_send_cmd("AT+RST",buf,BUF_SIZE,1);
  printf("\t%s\n",buf);
  delay_ms(500);
  //see if we are using WiFi
  if(wemo_config.standalone)
    return; 

  //set to mode STA
  printf("AT+CWMODE=1:");
  wifi_send_cmd("AT+CWMODE=1",buf,BUF_SIZE,1);
  printf("\t%s\n",buf);
  delay_ms(500);
  //try to join the specified network  
  sprintf(tx_buf,"AT+CWJA=\"%s\",\"%s\"",config.wifi_ssid,config.wifi_pwd);
  wifi_send_cmd(tx_buf,buf,BUF_SIZE,5);
  printf("\t%s\n",buf);
  //see if we have an IP address
  printf("AT+CIFSR");
  wifi_send_cmd("AT+CIFSR",buf,BUF_SIZE,2);
  printf("\t%s\n",buf);
};

uint8_t rx_buf [RESP_BUF_SIZE];
uint32_t rx_buf_idx = 0;
bool rx_wait = false;

int wifi_send_cmd(const char* cmd, char* resp, uint32_t maxlen, int timeout){
  rx_buf_idx = 0;
  uint32_t rx_start, rx_end;
  //enable RX interrupts
  usart_enable_interrupt(WIFI_UART, US_IER_RXRDY);
  NVIC_EnableIRQ(WIFI_UART_IRQ);
  //write the command
  usart_serial_write_packet(WIFI_UART,(uint8_t*)cmd,strlen(cmd));
  //terminate the command
  usart_serial_putchar(WIFI_UART,'\r');
  usart_serial_putchar(WIFI_UART,'\n');
  //wait for [timeout] seconds
  while(timeout>0){
    //start the timer
    rx_wait=true;
    tc_start(TC0, 0);	  
    //when timer expires, return what we have in the buffer
    while(rx_wait);
    tc_stop(TC0,0);
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
  //copy the data to the response buffer
  if((rx_end-rx_start+1)>maxlen){
    memcpy(resp,&rx_buf[rx_start],maxlen-1);
    resp[maxlen-1]=0x0;
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
  uint8_t tmp;
  usart_serial_getchar(WIFI_UART,&tmp);
  if(rx_buf_idx>=RESP_BUF_SIZE){
    printf("error!\n");
    return; //ERROR!!!!!
  }
  rx_buf[rx_buf_idx++]=(char)tmp;
}
