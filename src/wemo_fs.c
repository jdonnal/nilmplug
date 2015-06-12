#include <asf.h>
#include "wemo_fs.h"
#include "rtc.h"
#include "monitor.h"
#include "conf_membag.h"
#include <string.h>

FATFS fs;

uint8_t fs_init(void){
  Ctrl_status status;
  FRESULT res;
  FILINFO fno;

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
  //make sure we have a data directory
  if((res=f_stat("/data",&fno))!=FR_OK){
    if(res==FR_NO_FILE){ //create the directory
      if((res=f_mkdir("/data")))
	printf("[FAIL] could not make data dir\n");
    } else {
      printf("[FAIL] fstat %d\r\n",res);
    }
  }
  
  //load the configs
  fs_read_config();
  
  return 0;
}

void fs_read_config(void){
  FIL file;
  FRESULT res;
  char buf[50]; //string buffer
  int i;
  #define NUM_CONFIGS 7
  char *config_tags[NUM_CONFIGS] = {
    "serial_number: ","wifi_ssid: ","wifi_pwd: ","standalone: ",
    "mgr_url: ", "nilm_id: ", "nilm_ip_addr: "};
  char *config_vals[NUM_CONFIGS] = {
    wemo_config.serial_number, wemo_config.wifi_ssid, wemo_config.wifi_pwd,
    wemo_config.str_standalone, wemo_config.mgr_url, wemo_config.nilm_id, 
    wemo_config.nilm_ip_addr};
  //open the config file
  res = f_open(&file,CONFIG_FILE, FA_READ);
  if (res != FR_OK) {
    printf("Error reading config file: res %d\r\n", res);
    return;
  }
  // match the config tag against possible values,
  // if a match, load the value into our config  
  for(i=0;i<NUM_CONFIGS;i++){
    f_gets(buf,50,&file);
    if(strstr(buf,config_tags[i])==buf){
      memcpy(config_vals[i],&buf[strlen(config_tags[i])],
	     strlen(buf)-strlen(config_tags[i])-1); //extra 1 for \n
      continue;
    }
    if(i==NUM_CONFIGS-1){ //didn't find the config value
      sprintf(buf,"Error, missing config: [%s]",config_tags[i]);
      core_log(buf);
    }
  }
  //set the standalone config
  if(strstr(wemo_config.str_standalone,"true")==
     wemo_config.str_standalone)
    wemo_config.standalone = true;
  else
    wemo_config.standalone = false;
  return;
}



void fs_write_config(void){
  FIL file;
  FRESULT res;
  char buf[50]; //string buffer
  int i;
  UINT len;
  #define NUM_CONFIGS 7
  char *config_tags[NUM_CONFIGS] = {
    "serial_number: ","wifi_ssid: ","wifi_pwd: ","standalone: ",
    "mgr_url: ", "nilm_id: ", "nilm_ip_addr: "};
  char *config_vals[NUM_CONFIGS] = {
    wemo_config.serial_number, wemo_config.wifi_ssid, wemo_config.wifi_pwd,
    wemo_config.str_standalone, wemo_config.mgr_url, wemo_config.nilm_id, 
    wemo_config.nilm_ip_addr};
  //open the config file
  res = f_open(&file,CONFIG_FILE, FA_WRITE);
  if (res != FR_OK) {
    printf("Error opening config file: res %d\r\n", res);
    return;
  }
  //write each config out
  for(i=0;i<NUM_CONFIGS;i++){
    sprintf(buf,"%s%s\n",config_tags[i],config_vals[i]);
    f_write(&file,buf,strlen(buf),&len);
  }
  f_close(&file);
}


void fs_log(const char* content){
  int buf_size = MD_BUF_SIZE;
  char *msg_buf;
  char *ts_buf;
  UINT len;
  FIL log_file;
  FRESULT res;

  //open the log file
  res = f_open(&log_file, LOG_FILE,
	       FA_OPEN_ALWAYS | FA_WRITE);
  /* Move to end of the file to append data */
  res = f_lseek(&log_file, f_size(&log_file));
  if (res != FR_OK) {
    printf("[FAIL] res %d\r\n", res);
  }
  //allocate memory
  msg_buf = core_malloc(buf_size);
  ts_buf = core_malloc(buf_size);
  rtc_get_time_str(ts_buf,buf_size);
  snprintf(msg_buf,buf_size,"[%s]: %s\n",ts_buf,content);
  f_write(&log_file,msg_buf,strlen(msg_buf),&len);
  //close the log file
  f_close(&log_file);
  //free memory
  core_free(msg_buf);
  core_free(ts_buf);
  fs_write_power_pkt(NULL);
}

void fs_write_power_pkt(const power_pkt* pkt){

  UINT len;
  FIL data_file;
  FRESULT res;

  //open the data file
  res = f_open(&data_file, LOG_FILE,
	       FA_OPEN_ALWAYS | FA_WRITE);
  /* Move to end of the file to append data */
  res = f_lseek(&data_file, f_size(&data_file));
  if (res != FR_OK) {
    printf("[FAIL] res %d\r\n", res);
  }
  f_write(&data_file,pkt,sizeof(power_pkt),&len);
  //close the data file
  f_close(&data_file);
}

void fs_info(void){
  //print out file and sizes
  FRESULT res;
  FILINFO fno;
  DIR dir;
  char *fn;  
  char *ts_buf;
  int TS_BUF_SIZE = MD_BUF_SIZE;
  ts_buf = core_malloc(TS_BUF_SIZE);
  res = f_opendir(&dir, "/");                       /* Open the directory */
  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &fno);                   /* Read a directory item */
      if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
      if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
      fn = fno.fname;
      
      snprintf(ts_buf,TS_BUF_SIZE,"%u/%02u/%02u, %02u:%02u",
	       (fno.fdate >> 9) + 1980, fno.fdate >> 5 & 15, fno.fdate & 31,
	       fno.ftime >> 11, fno.ftime >> 5 & 63);
	       
      printf("%s %ld %s\n",fn,fno.fsize,ts_buf);
    }
  }
  core_free(ts_buf);
}




