#include <asf.h>
#include "wemo_fs.h"
#include <string.h>

struct config {
  char wifi_ssid[20];     //the wireless network to connect to
  char wifi_pwd[30];      //the network password (may be blank)
  uint8_t standalone;     //TRUE: don't try to connect to a network
  char mgr_url[30];       //URL of managment node (www.wattsworth.net)
  char wattsworth_id[30]; //ID wattsworth owner (nilm9F59)
  char wattsworth_ip[30]; //cached IP address of wattsworth owner
} wemo_config;


FATFS fs;
FIL log_file;

uint8_t wemo_fs_init(void){
  Ctrl_status status;
  FRESULT res;

  char file_name[30]; //buffer to hold file names

  //initialize the HSCMI controller
  sd_mmc_init();
  //install the SD Card
  do {
    status = sd_mmc_test_unit_ready(0);
    if (CTRL_FAIL == status) {
      printf("Card install FAIL\n");
      printf("Please unplug and re-plug the card.\n");
      while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
      }
    }
  } while (CTRL_GOOD != status);
  //mount the FS
  memset(&fs, 0, sizeof(FATFS));
  res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
  if (FR_INVALID_DRIVE == res) {
    printf("[FAIL] res %d\r\n",res);
  }
  //load the configs
  wemo_read_config();
  
  //open the log file
  memcpy(file_name,LOG_FILE,strlen(CONFIG_FILE));
  file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
  res = f_open(&log_file,
	       (char const *)file_name,
	       FA_OPEN_ALWAYS | FA_WRITE);
  if (res != FR_OK) {
    printf("[FAIL] res %d\r\n", res);
    return false;
  }
  return 0;
}

void wemo_read_config(void){
  FIL file;
  FRESULT res;
  char buf[30]; //string buffer
  char *config_tag; //holds what we're looking for in the config file
  //open the config file
  res = f_open(&file,CONFIG_FILE, FA_READ);
  if (res != FR_OK) {
    printf("[FAIL] res %d\r\n", res);
    return;
  }
  //////////////////////////
  //Load the config struct
  //
  // 1.) check the config tag is correct (order matters!)
  // 2.) copy the value to the config struct- remove trailing newline
  
  // WiFi SSID
  f_gets(buf,30,&file);
  config_tag = "wifi_ssid: ";
  if(strstr(buf,config_tag)!=buf)
    goto config_error;
  memcpy(wemo_config.wifi_ssid,&buf[strlen(config_tag)],
	 strlen(buf)-strlen(config_tag)-1);
  // WiFi Password
  config_tag = "wifi_pwd: ";
  if(strstr(buf,config_tag)!=buf)
    goto config_error;
  memcpy(wemo_config.wifi_pwd,&buf[strlen(config_tag)],
	 strlen(buf)-strlen(config_tag)-1);
  // Standlone mode (boolean)
  f_gets(buf,30,&file);
  config_tag = "standalone: ";
  if(strstr(buf,config_tag)!=buf)
    goto config_error;
  if(strstr(buf,"false"))   //look for true or false
    wemo_config.standalone=false;
  else if(strstr(buf,"true"))
    wemo_config.standalone=true;
  else
    goto config_error;
  // Manager URL
  f_gets(buf,30,&file);
  config_tag = "mgr_url: ";
  if(strstr(buf,config_tag)!=buf)
    goto config_error;
  memcpy(wemo_config.mgr_url,&buf[strlen(config_tag)],strlen(buf)-strlen(config_tag)-1);
  // Wattsworth ID
  f_gets(buf,30,&file);
  config_tag = "wattsworth_id: ";
  if(strstr(buf,config_tag)!=buf)
    goto config_error;
  memcpy(wemo_config.wattsworth_id,&buf[strlen(config_tag)],strlen(buf)-strlen(config_tag)-1);
 
  return; 
 config_error:
  printf("Error loading config\n");
  return;
}


void wemo_log(const char* content){
  //TODO
  printf(content);
}


void wemo_write_config(void){
  printf("saving config\n");
}
