#ifndef __WEMO_FS__
#define __WEMO_FS__


#define CONFIG_FILE "config.txt"
#define LOG_FILE    "log.txt"

typedef struct config_struct {
  char wifi_ssid[20];     //the wireless network to connect to
  char wifi_pwd[30];      //the network password (may be blank)
  uint8_t standalone;     //TRUE: don't try to connect to a network
  char mgr_url[30];       //URL of managment node (www.wattsworth.net)
  char wattsworth_id[30]; //ID wattsworth owner (nilm9F59)
  char wattsworth_ip[30]; //cached IP address of wattsworth owner
  char wemo_ip[30];       //currently assigned IP address
} config;

extern config wemo_config;

uint8_t wemo_fs_init(void);
//char* wemo_fs_get_config(const char* key);
//void wemo_fs_set_config(const char* key, const char* value);
void wemo_write_config(void);
void wemo_read_config(void);
void wemo_log(const char* content);

#endif
