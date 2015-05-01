#include <asf.h>
#include "string.h"
#include "server.h"
#include "monitor.h"
#include "wemo_fs.h"

/**********************************
------Theory of Operation----------
The server listens for relay commands over TCP
and pushes out updates from the power meter over TCP
-----Relay Commands-------
relay_on: turn the relay on
relay_off: turn the relay off
----Power Meter----------
Read the wemo power chip every second. Use timer 1 to generate
interrupts, when interrupt fires, check if data in the power struct
is valid, if so send it //otherwise flush it and start listening to
the UART. The UART waits for the sync byte 0xAE then reads 29 more
bytes computes the checksum and parses the data into the power
struct if the checksum is good and everything is stored the valid
flag is set
************************************/


power_pkt wemo_power;
uint8_t wemo_buffer [30];

void server_init(void){
  //allocate memory for the server buffer
  //*****CHECK THIS CODE!!!*****
  server_buf = (char*)malloc(SERVER_BUF_SIZE);
  server_buf_len=SERVER_BUF_SIZE;
  //set up the power meter UART
  static usart_serial_options_t usart_options = {
    .baudrate = WEMO_UART_BAUDRATE,
    .charlength = WEMO_UART_CHAR_LENGTH,
    .paritytype = WEMO_UART_PARITY,
    .stopbits = WEMO_UART_STOP_BITS
  };
  gpio_configure_pin(PIO_PB2_IDX, (PIO_PERIPH_A | PIO_DEFAULT));
  pmc_enable_periph_clk(ID_WEMO_UART);
  sysclk_enable_peripheral_clock(ID_WEMO_UART);
  usart_serial_init(WEMO_UART,&usart_options);
  NVIC_EnableIRQ(WEMO_UART_IRQ);

  // Trigger from timer 1
  pmc_enable_periph_clk(ID_TC1);
  tc_init(TC0, 1,                        // channel 1
	  TC_CMR_TCCLKS_TIMER_CLOCK5     // source clock (CLOCK5 = Slow Clock)
	  | TC_CMR_CPCTRG                // up mode with automatic reset on RC match
	  | TC_CMR_WAVE                  // waveform mode
	  | TC_CMR_ACPA_CLEAR            // RA compare effect: clear
	  | TC_CMR_ACPC_SET              // RC compare effect: set
	  );
  TC0->TC_CHANNEL[1].TC_RA = 0;  // doesn't matter
  TC0->TC_CHANNEL[1].TC_RC = 64000;  // sets frequency: 32kHz/32000 = 1 Hz
  NVIC_EnableIRQ(TC1_IRQn);
  tc_enable_interrupt(TC0, 1, TC_IER_CPCS);
  tc_start(TC0,1);
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

uint8_t process_packet(uint8_t *buffer){
  //process 30 byte data packet buffer
  uint8_t checksum = 0;
  uint8_t bytes[3];
  int32_t vals[9];
  int i;
  //1.) check for header
  if(buffer[0]!=0xAE){
    return false;
  }
  //2.) compute checksum
  for(i=0;i<29;i++){
    checksum += buffer[i];
  }
  checksum = (~checksum)+1;
  if(checksum!=buffer[29])
    return false;
  //3.) Parse raw data into values
  //    Data is 3 byte signed LSB
  for(i=0;i<9;i++){
    bytes[0] = buffer[3*i+2];
    bytes[1] = buffer[3*i+3];
    bytes[2] = buffer[3*i+4];
    vals[i] = bytes[0] | bytes[1]<<8 | bytes[2]<<16;
    if((bytes[2]&0x08)==0x08){
      vals[i] |= 0xFF << 24; //sign extend top byte
    }
  }
  //4.) Populate the power struct
  wemo_power.vrms = vals[2];
  wemo_power.irms = vals[3];
  wemo_power.watts= vals[4];
  wemo_power.pavg = vals[5];
  wemo_power.pf = vals[6];
  wemo_power.freq   = vals[7];
  wemo_power.kwh  = vals[8];
  //5.) Set the valid flag
  wemo_power.valid = true;
  //all done
  return true;
};

ISR(TC1_Handler)
{
  //check if there is a valid data packet
  if(wemo_power.valid==true){
    core_log_power_data(&wemo_power);
  }
  wemo_power.valid=false;
  //now start listening to the WEMO
  usart_enable_interrupt(WEMO_UART, US_IER_RXRDY);
  //clear the interrupt so we don't get stuck here
  tc_get_status(TC0,1);
}

ISR(UART1_Handler)
{
  uint8_t tmp;
  static uint8_t pkt[30]; //30 byte packet
  static uint8_t pkt_idx;
  usart_serial_getchar(WEMO_UART,&tmp);
  switch(pkt_idx){
  case 0: //search for sync byte
    if(tmp==0xAE){
      //found sync byte, start capturing the packet
      pkt[pkt_idx++]=tmp;
    }
    break;
  case 30: //packet is full, read checksum and return data
    if(process_packet(pkt)){
      //succes, stop listening to the UART
      usart_disable_interrupt(WEMO_UART, US_IER_RXRDY);
      //reset the index
      pkt_idx=0;
    } else { //failure, log it and look for the next packet
      wemo_log("bad packet");
      pkt_idx=0;
    }
    break;
  default: //reading packet
    pkt[pkt_idx++]=tmp;
  }
}
