#include <asf.h>
#include "wemo_fs.h"
#include "rtc.h"

#include <string.h>

config wemo_config;

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
  res = f_open(&log_file, LOG_FILE,
	       FA_OPEN_ALWAYS | FA_WRITE);
  /* Move to end of the file to append data */
  res = f_lseek(&log_file, f_size(&log_file));
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
  char *msg_buf,*ts_buf;
  UINT len;
  msg_buf = (char*)malloc(strlen(content)+30);
  ts_buf = (char*)malloc(30);
  rtc_get_time_str(ts_buf);
  sprintf(msg_buf,"[%s]: %s\n",ts_buf,content);
  f_write(&log_file,msg_buf,strlen(msg_buf),&len);
  //always sync log entries
  f_sync(&log_file);
}


void wemo_write_config(void){
  printf("saving config\n");
}
