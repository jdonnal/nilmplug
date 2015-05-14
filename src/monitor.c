#include "asf.h"
#include "debug.h"
#include "string.h"
#include "usb.h"
#include "types.h"
#include "monitor.h"
#include "rtc.h"
#include "wemo.h"
#include "wifi.h"
#include "wemo_fs.h"
#include "rgb_led.h"
#include "conf_membag.h"

//data structures and functions for 
//command line interaction
#define CMD_BUF_SIZE MD_BUF_SIZE
#define WHITESPACE "\t\r\n "
#define MAXARGS 16
static int runcmd(char *buf);
char *cmd_buf;
int cmd_buf_idx = 0;
bool cmd_buf_full = false; //flag when a command has been rx'd
bool b_usb_enabled = false;

//ping pong packet structs for filling and transmitting
power_pkt power_pkts[2];
power_pkt *cur_pkt, *tx_pkt;
//data structure for wemo configuration
config wemo_config;

//Functions that *request* data from an outside server must register
//a callback that core_process_wifi_data calls when data is received
void (*tx_callback)(char*);

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv);
};

static struct Command commands[] = {
  { "help","Display this list of commands", mon_help},
  { "rtc", "get or set RTC", mon_rtc},
  { "relay", "turn relay on",mon_relay},
  { "echo", "turn echo on or off",mon_echo},
  { "config", "get or set a config",mon_config},
  { "log", "read or clear the log", mon_log},
  { "memory", "show memory statistics", mon_memory},
  { "restart", "restart [bootloader]", mon_restart}
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

int
mon_help(int argc, char **argv)
{
  int i;
  for (i = 0; i < NCOMMANDS; i++)
    printf("%s - %s\n", commands[i].name, commands[i].desc);
  return 0;
}

int
mon_rtc(int argc, char **argv){
  int time_buf_size =  MD_BUF_SIZE;
  char *time_buf;
  uint8_t yr, dt, mo, dw, hr, mn, sc;
  if(argc<2){
    printf("wrong number of args to set time\n");
    return -1;
  }
  if(strcmp(argv[1],"get")==0){
    time_buf = membag_alloc(time_buf_size);
    if(time_buf==NULL)
      core_panic();
    rtc_get_time_str(time_buf,time_buf_size);
    printf("Time: %s\n",time_buf);
    membag_free(time_buf);
  }
  else if(strcmp(argv[1],"set")==0){
    if(argc!=9){
      printf("please specify a time\n");
      return -1;
    }
    yr = atoi(argv[2]);
    mo = atoi(argv[3]);
    dt = atoi(argv[4]);
    dw = atoi(argv[5]);
    hr = atoi(argv[6]);
    mn = atoi(argv[7]);
    sc = atoi(argv[8]);
    if(i2c_rtc_set_time(sc,mn,hr,dw,dt,mo,yr)!=0)
      printf("error setting RTC\n");
    else{
      time_buf = membag_alloc(time_buf_size);
      if(time_buf==NULL)
	core_panic();
      rtc_get_time_str(time_buf,time_buf_size);
      printf("Set time to: %s\n",time_buf);
      membag_free(time_buf);
    }
  }
  else{
    printf("bad arguments to rtc\n");
    return -1;
  }
  return 0;
}

//set the relay and store the new state in 
//the backup register in case of software reset
int
mon_relay(int argc, char **argv){
  int state;
  state = gpbr_read(GPBR_RELAY_STATE);
  if(argc==1){ //print the relay state
    if(state)
      printf("relay [on]\n");
    else
      printf("relay [off]\n");
    return -1;
  }
  if(argc!=2){
    printf("relay [on|off|toggle]\n");
    return -1;
  }
  if(strcmp(argv[1],"on")==0){
    gpio_set_pin_high(RELAY_PIN);
    gpbr_write(GPBR_RELAY_STATE,1);
  }
  else if(strcmp(argv[1],"off")==0){
    gpio_set_pin_low(RELAY_PIN);
    gpbr_write(GPBR_RELAY_STATE,0);
  }
  else if(strstr(argv[1],"toggle")==argv[1]){
    gpio_toggle_pin(RELAY_PIN);
    if(state==0)
      state = 1;
    else
      state = 0;
    gpbr_write(GPBR_RELAY_STATE,state);
  }
  else{
    printf("bad argument to relay\n");
    return -1;
  }
  return 0;
}

int 
mon_echo(int argc, char **argv){
  if(argc!=2){
    printf("wrong number of args to relay\n");
    return -1;
  }
  if(strcmp(argv[1],"on")==0)
    wemo_config.echo = true;
  else if(strcmp(argv[1],"off")==0)
    wemo_config.echo = false;
  else{
    printf("bad argument to echo\n");
    return -1;
  }
  return 0;
}

int
mon_config(int argc, char **argv){
  int i;
  //array of gettable configs and their values
  #define GETTABLE_CONFIG_SIZE 7
  char *gettable_configs[GETTABLE_CONFIG_SIZE]= {
    "wifi_ssid","mgr_url","serial_number",
    "ip_addr","nilm_id","nilm_ip","standalone"};
  char *gettable_config_vals[GETTABLE_CONFIG_SIZE] = {
    wemo_config.wifi_ssid, wemo_config.mgr_url, wemo_config.serial_number,
    wemo_config.ip_addr, wemo_config.nilm_id, wemo_config.nilm_ip_addr,
    wemo_config.str_standalone};
  //array of settable configs and their values
  #define SETTABLE_CONFIG_SIZE 7 
  char *settable_configs[SETTABLE_CONFIG_SIZE]= {
    "wifi_ssid","wifi_pwd","mgr_url","serial_number",
    "nilm_id","standalone","nilm_ip"};
  char *settable_config_vals[SETTABLE_CONFIG_SIZE] = {
    wemo_config.wifi_ssid, wemo_config.wifi_pwd, wemo_config.mgr_url, 
    wemo_config.serial_number, wemo_config.nilm_id, 
    wemo_config.str_standalone, wemo_config.nilm_ip_addr};

  if(argc<2){
    printf("wrong number of args to config\n");
    return -1;
  }
  //--- request to get a config ----
  if(strcmp(argv[1],"get")==0){
    if(argc!=3){
      printf("specify a config to read\n");
      return -1;
    }
    //find the matching config
    for(i=0;i<GETTABLE_CONFIG_SIZE;i++){
      if(strstr(argv[2],gettable_configs[i])==argv[2]){
	printf(gettable_config_vals[i]);
	printf("\n");
	return 0;
      }
    } //couldn't find config
    printf("Error, config [%s] is not available\n",argv[2]);
    return -1;
  }
  //--- request to set a config ----
  if(strcmp(argv[1],"set")==0){
    if(argc<3){
      //clear the specified config
      printf("specify a config and the value");
    }
    //find the matching config
    for(i=0;i<SETTABLE_CONFIG_SIZE;i++){
      if(strstr(argv[2],settable_configs[i])==argv[2]){
	if(argc==4 && strlen(argv[3])>MAX_CONFIG_LEN){
	  printf("requested value is too large to store\n");
	  return -1;
	}
	//clear out the existing config
	memset(settable_config_vals[i],0x0,MAX_CONFIG_LEN);
	//save the config to the config struct, if no value is specified
	//leave it cleared
	if(argc==4)
	  memcpy(settable_config_vals[i],argv[3],strlen(argv[3]));
	//if echo is on, read back the result
	if(wemo_config.echo){
	  printf("Set [%s] to [%s]\n",argv[2],settable_config_vals[i]);
	}
	//save the new config
	fs_write_config();
      }
    }
  }
  return 0;
}

int
mon_log(int argc, char **argv){
  FIL fil;
  int line_buf_size = MD_BUF_SIZE;
  char *line_buf;
  FRESULT fr; 

  if(argc!=2){
    printf("specify [read] or [erase]\n");
    return -1;
  }
  if(strcmp(argv[1],"read")==0){
    //allocate the line buffer
    line_buf = membag_alloc(line_buf_size);
    if(line_buf==NULL)
      core_panic();
    //print the log out to STDOUT
    fr = f_open(&fil, LOG_FILE, FA_READ);
    if(fr){
      printf("error reading log: %d\n",(int)fr);
      membag_free(line_buf);
      return -1;
    }
    while(f_gets(line_buf, line_buf_size, &fil)){
      printf("%s",line_buf);
    }
    f_close(&fil);
    membag_free(line_buf);
    return 0;
  }
  else if(strcmp(argv[1],"erase")==0){
    fr = f_open(&fil, LOG_FILE, FA_WRITE);
    if(fr){
      printf("error erasing log: %d\n", (int)fr);
      return -1;
    }
    f_lseek(&fil,0);
    f_truncate(&fil);
    f_close(&fil);
    if(wemo_config.echo)
      printf("erased log\n");
    return 0;
  }
  else{
    printf("specify [read] or [erase]\n");
    return -1;
  }
  //shouldn't get here
  return 0;
}

int 
mon_restart(int argc, char **argv){
  if(argc==2){
    //check for [bootloader] flag
    if(strcmp(argv[1],"bootloader")==0){
      //set the gpnvm bit to atmel bootloader
      efc_perform_command(EFC0, EFC_FCMD_CGPB, 1);
    }
    else{
      printf("[bootloader] to reboot with Atmel bootloader\n");
      return -1;
    }
  }
  //detach USB
  //  udc_detach();
  //software reset
  rstc_start_software_reset(RSTC);
  return 0;
}

int 
mon_memory(int argc, char **argv){
  size_t total, free;
  total = membag_get_total();
  free = membag_get_total_free();
  printf("Allocated %d of %d bytes (%d%%)\n",total-free,total,((total-free)*100)/total);
  printf("Largest free block: %d bytes\n",membag_get_largest_free_block_size());
  printf("Smallest free block: %d bytes\n",membag_get_smallest_free_block_size());
  return 0;
}
/***** Core commands ****/

void core_process_wifi_data(void){
  int BUF_SIZE=XL_BUF_SIZE;
  char *buf;
  int chan_num, data_size, r;
  //match against the data
  if(strlen(wifi_rx_buf)>BUF_SIZE){
    printf("can't process rx'd packet, too large\n");
    core_log("can't process rx'd packet, too large");
    return;
  }
  //allocate memory
  buf = core_malloc(BUF_SIZE);
  r=sscanf(wifi_rx_buf,"\r\n+IPD,%d,%d:%s", &chan_num, &data_size, buf);
  if(r!=3){
    printf("rx'd corrupt data, ignoring\n");
    core_log("rx'd corrupt data, ignoring\n");
    //free memory
    core_free(buf);
    return;
  }
  printf("Got [%d] bytes on channel [%d]\n",
    data_size, chan_num);
  //discard responses from the NILM to power logging packets, but keep the response
  //if another core function has requested some data, this is done with the callback 
  //function. The requesting core function registers a callback and this function calls
  //it and then resets the callback to NULL
  if(chan_num==4){
    if(tx_callback==NULL){
      //clear the server buffer
      memset(wifi_rx_buf,0x0,WIFI_RX_BUF_SIZE);
      //close the socket
      wifi_send_cmd("AT+CIPCLOSE=4","Unlink",buf,BUF_SIZE,1);
      //free memory
      core_free(buf);
      return;
    } else{
      //execute the callback
      (*tx_callback)(wifi_rx_buf);
      tx_callback=NULL;
    }
  }
  //this data must be inbound to the server port, process the command
  if(strcmp(buf,"relay_on")==0){
    gpio_set_pin_high(RELAY_PIN);
    printf("relay ON\n");
  }
  else if(strcmp(buf,"relay_off")==0){
    gpio_set_pin_low(RELAY_PIN);
    printf("relay OFF\n");
  }
  else{
    printf("unknown command: %s\n",buf);
    wifi_send_data(0,"error: unknown command");
    //free memory
    core_free(buf);
    return;
  }
  //clear the server buffer
  memset(wifi_rx_buf,0x0,WIFI_RX_BUF_SIZE);
  //return "OK" to indicate success
  wifi_send_data(0,"OK");
}

void core_wifi_link(void){
  printf("linked!\n");
  //clear the server buffer
  memset(wifi_rx_buf,0x0,WIFI_RX_BUF_SIZE);
}

void core_wifi_unlink(void){
  printf("unlinked\n");
  //clear the server buffer
  memset(wifi_rx_buf,0x0,WIFI_RX_BUF_SIZE);
}

void core_transmit_power_packet(power_pkt *wemo_pkt){
  int r, i, j;
  int TX_BUF_SIZE = XL_BUF_SIZE;
  int PAYLOAD_BUF_SIZE = XL_BUF_SIZE;
  int DATA_BUF_SIZE = MD_BUF_SIZE;
  char *tx_buf, *payload_buf;
  char *bufs[7];
  int32_t *srcs[7] = {wemo_pkt->vrms, wemo_pkt->irms, wemo_pkt->pavg, wemo_pkt->watts,
		    wemo_pkt->pf, wemo_pkt->freq, wemo_pkt->kwh};
  char *buf; int32_t *src;
  
  //make sure all the data is present
  if(wemo_pkt->status != POWER_PKT_READY){
    printf("Error, packet is not ready to tx!\n");
    return;
  }
  //now we are filling up the POST request
  wemo_pkt->status = POWER_PKT_TX_IN_PROG;

  //allocate memory for the POST request
  tx_buf = core_malloc(TX_BUF_SIZE);
  payload_buf = core_malloc(DATA_BUF_SIZE);

  //print out the data arrays as strings
  for(j=0;j<7;j++){
    src = srcs[j];
    buf = core_malloc(DATA_BUF_SIZE);
    bufs[j] = buf;
    for(i=0;i<PKT_SIZE;i++){
      snprintf(buf+strlen(buf),DATA_BUF_SIZE,"%li",src[i]);
      if(i<(PKT_SIZE-1))
	snprintf(buf+strlen(buf),DATA_BUF_SIZE,", ");
    }
  }
  //stick them in a json format
  snprintf(payload_buf,PAYLOAD_BUF_SIZE,
	   "{\"plug\":\"%s\",\"ip\":\"%s\",\"time\":\"%s\","
	   "\"vrms\":[%s],\"irms\":[%s],\"watts\":[%s],\"pavg\":[%s],"
	   "\"pf\":[%s],\"freq\":[%s],\"kwh\":[%s]}",
	   wemo_config.serial_number,wemo_config.ip_addr,wemo_pkt->timestamp,
	   bufs[0],bufs[1],bufs[2],bufs[3],bufs[4],bufs[5],bufs[6]);
  snprintf(tx_buf,TX_BUF_SIZE,
	     "POST /config/plugs/log HTTP/1.1\r\n"
	     "User-Agent: WemoPlug\r\n"
	     "Host: NILM\r\nAccept:*/*\r\n"
	     "Connection: keep-alive\r\n"
	     "Content-Type: application/json\r\n"
	     "Content-Length: %d\r\n"
	     "\r\n%s",
	     strlen(payload_buf),payload_buf);
  //send the packet!
  r = wifi_transmit(wemo_config.nilm_ip_addr,80,tx_buf);
  switch(r){
  case TX_SUCCESS:
    wemo_pkt->status = POWER_PKT_EMPTY;
    break;
  case TX_BAD_DEST_IP: //if we can't find NILM, get its IP from the manager
    wemo_pkt->status = POWER_PKT_EMPTY; //just dump the packet
    core_get_nilm_ip_addr();
    break;
  default:
    wemo_pkt->status = POWER_PKT_TX_FAIL;
  }
  //free memory
  core_free(tx_buf);
  core_free(payload_buf);
  for(j=0;j<7;j++){
    core_free(bufs[j]);
  }
}

/////////////////////////
// core_log_power_data
//     Add a power_sample to the current power_pkt
//     When the pkt is full switch to the other pkt
//     (ping pong buffer). Throws an error if the 
//     next buffer is not empty (still being TX'd)
void core_log_power_data(power_sample *sample){
  static int wemo_sample_idx = 0;
  power_pkt *tmp_pkt;
  //check for error conditions
  switch(cur_pkt->status){
  case POWER_PKT_EMPTY:
    if(wemo_sample_idx!=0){
      printf("Error, empty packet but nonzero index\n");
      return;
    }
    //set the timestamp for the packet with the first one
    rtc_get_time_str(cur_pkt->timestamp,PKT_TIMESTAMP_BUF_SIZE);
    break;
  case POWER_PKT_TX_IN_PROG:
    printf("Error, this packet is still being transmitted\n");
    return;
  case POWER_PKT_TX_FAIL:
    printf("Error, this packet failed TX and isn't clean\n");
    break;
  case POWER_PKT_READY:
    printf("Transmit this packet first!\n");
    return;
  }
  cur_pkt->status = POWER_PKT_FILLING;
  cur_pkt->vrms[wemo_sample_idx]  = sample->vrms;
  cur_pkt->irms[wemo_sample_idx]  = sample->irms;
  cur_pkt->watts[wemo_sample_idx] = sample->watts;
  cur_pkt->pavg[wemo_sample_idx]  = sample->pavg;
  cur_pkt->freq[wemo_sample_idx]  = sample->freq;
  cur_pkt->pf[wemo_sample_idx]    = sample->pf;
  cur_pkt->kwh[wemo_sample_idx]   = sample->kwh;
  wemo_sample_idx++;
  if(wemo_sample_idx==PKT_SIZE){
    cur_pkt->status=POWER_PKT_READY;
    //switch buffers
    tmp_pkt = cur_pkt;
    cur_pkt = tx_pkt;
    tx_pkt = tmp_pkt;
    //start filling the new buffer
    wemo_sample_idx = 0;
  }
}


///////////////////
// core_get_nilm_ip_addr
//    Request the NILM IP address from the
//    manager, this allows us to operate in 
//    DHCP environments because the manager URL
//    can be resolved by DNS to a static IP
//
void
core_get_nilm_ip_addr(void){
  int BUF_SIZE=MD_BUF_SIZE;
  char *buf;
  //allocate memory
  buf=core_malloc(BUF_SIZE);
  snprintf(buf,BUF_SIZE,
	   "GET /nilm/get_ip?serial_number=%s HTTP/1.1\r\n"
	   "User-Agent: WemoPlug\r\n"
	   "Host: %s\r\n"
	   "Accept:*/*\r\n\r\n",
	   wemo_config.serial_number,wemo_config.mgr_url);
  wifi_transmit(wemo_config.mgr_url,80,buf);
  tx_callback = &core_get_nilm_ip_addr_cb;
  //free memory
  core_free(buf);
}
////////////////////
//callback function
void core_get_nilm_ip_addr_cb(char* data){
  printf("got the data!\n");
}

///////////////////
// Core Filesystem commands
//
void 
core_log(const char* content){
  //log to the SD Card
  fs_log(content);
}

////////////////////
// Core USB commands
//
void 
core_usb_enable(uint8_t port, bool b_enable){
  cmd_buf_idx=0;
  memset(cmd_buf,0x0,CMD_BUF_SIZE);
  if(b_enable){
    wemo_config.echo = true;
    b_usb_enabled = true;
    delay_ms(500);
    printf("Wattsworth WEMO(R) Plug v1.0\n");
    printf("  [help] for more information\n");
    printf("> ");
  } 
  else {
    b_usb_enabled = false;
  }
}

void core_putc(void* stream, char c){
  if(b_usb_enabled){
    while(!udi_cdc_is_tx_ready());
    udi_cdc_write_buf(&c,1);
  }
}


/***** Kernel monitor command interpreter *****/

/****************
 * This is the core loop
 * Uses a tick to schedule operations
 *****************/
uint8_t sys_tick=0;
bool wifi_on = true; 

void monitor(void){
  uint32_t prev_tick=0;

  //allocate memory for buffers and flush them
  cmd_buf = membag_alloc(CMD_BUF_SIZE);
  if(!cmd_buf)
    core_panic();
  memset(cmd_buf,0x0,CMD_BUF_SIZE);
  //initialize the power packet buffers
  tx_pkt = &power_pkts[0];
  cur_pkt = &power_pkts[1];
  //both are empty
  tx_pkt->status = POWER_PKT_EMPTY;
  cur_pkt->status = POWER_PKT_EMPTY;

  //initialize runtime configs
  wemo_config.echo = false;
  //check if we are on USB
  if(gpio_pin_is_high(VBUS_PIN)){
    rgb_led_set(0,30,200); //blue light
    //don't use WiFi b/c we don't have the power
    //    wifi_on=false;
  }
  //check if reset is pressed
  if(gpio_pin_is_low(BUTTON_PIN)){
    //erase the config
    //TODO
    //spin until button is released
    while(gpio_pin_is_low(BUTTON_PIN)){
      rgb_led_set(255,255,0);
      delay_ms(250);
      rgb_led_set(0,0,0);
      delay_ms(250);
    }
    rgb_led_set(255,255,0);
  }
  //setup WIFI
  if(wifi_on){
    if(wifi_init()!=0){
      wifi_on=false;
      rgb_led_set(125,0,125);
    }
    else{
      //good to go! turn light green
      rgb_led_set(0,125,30);
    }
  }
  //initialize the wifi_rx buffer and flag
  wifi_rx_buf_full = false;
  memset(wifi_rx_buf,0x0,WIFI_RX_BUF_SIZE);
  while (1) {
    //try to send a packet if we are using WiFi
    if(tx_pkt->status==POWER_PKT_READY && wifi_on){
      core_transmit_power_packet(tx_pkt);
      if(tx_pkt->status==POWER_PKT_TX_FAIL){
	printf("resetting wifi\n");
	core_log("resetting wifi");
	wifi_init();
	//try again (only time for 2 tries)
	tx_pkt->status=POWER_PKT_READY;
	core_transmit_power_packet(tx_pkt);
      }
      //clear out the packet so we can start again
      tx_pkt->status=POWER_PKT_EMPTY;
    }
    //check for pending data from the Internet
    if(wifi_rx_buf_full){
      core_process_wifi_data();
      wifi_rx_buf_full=false;
    }
    //see if we have any commands to run
    if(cmd_buf_full){
      runcmd(cmd_buf); //  run it
      //clear the buffer
      cmd_buf_idx = 0;
      memset(cmd_buf,0x0,CMD_BUF_SIZE);
      if(wemo_config.echo)
	printf("\r> "); //print the prompt
      cmd_buf_full=false;
    }
    //reset watchdog
    if(sys_tick!=prev_tick){
      prev_tick=sys_tick;
      wdt_restart(WDT);
    }
  }
    
}

//Priority 3 (lowest)
ISR(PWM_Handler)
{
  sys_tick++;
  //check if there is a valid wemo sample
  if(wemo_sample.valid==true && wifi_on){
    core_log_power_data(&wemo_sample);
  }
  //take a wemo reading
  wemo_read_power();
  //clear the interrupt so we don't get stuck here
  pwm_channel_get_interrupt_status(PWM);  
}

void
core_read_usb(uint8_t port)
{
  uint8_t c;
  //check for incoming USB data
  while (udi_cdc_is_rx_ready()) {
    c = udi_cdc_getc();
    if(cmd_buf_full)
      return; //still processing last command
    if (c < 0) {
      printf("read error: %d\n", c);
      return;
    } else if ((c == '\b' || c == '\x7f') && cmd_buf_idx > 0) {
      if (wemo_config.echo)
	udi_cdc_putc('\b');
      cmd_buf_idx--;
    } else if (c >= ' ' && cmd_buf_idx < CMD_BUF_SIZE-1) {
      if(wemo_config.echo)
	udi_cdc_putc(c);
      cmd_buf[cmd_buf_idx++] = c;
    } else if (c == '\n' || c == '\r') {
      cmd_buf[cmd_buf_idx] = 0; //we have a complete command
      if(wemo_config.echo)
	printf("\r\n");
      //run the command in the main loop (get out of USB interrupt ctx)
      cmd_buf_full = true;
    }
  }
}

static int
runcmd(char *buf)
{
  int argc;
  char *argv[MAXARGS];
  uint16_t i;
  // Parse the command buffer into whitespace-separated arguments
  argc = 0;
  argv[argc] = 0;
  while (1) {
    // gobble whitespace
    while (*buf && strchr(WHITESPACE, *buf))
      *buf++ = 0;
    if (*buf == 0)
      break;
    
    // save and scan past next arg
    if (argc == MAXARGS-1) {
      printf("Too many arguments (max 16)\n");
      return 0;
    }
    argv[argc++] = buf;
    while (*buf && !strchr(WHITESPACE, *buf))
      buf++;
  }
  argv[argc] = 0;
  
  // Lookup and invoke the command
  if (argc == 0)
    return 0;
  for (i = 0; i < NCOMMANDS; i++) {
    if (strcmp(argv[0], commands[i].name) == 0){
      return commands[i].func(argc, argv);
    }
  }
  printf("Unknown command '%s'\n", argv[0]);
  return 0;
}

//Core Panic
//  set light red, spin forever
void 
core_panic(void){
  rgb_led_set(255,0,0);
  while(1);
}

//Core memory functions
void*
core_malloc(int size){
  void* ptr;
  ptr = membag_alloc(size);
  if(ptr==NULL)
    core_panic();
  memset(ptr,0x0,size);
  return ptr;
}

void
core_free(void *ptr){
  membag_free(ptr);
}
  
