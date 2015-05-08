#ifndef __WEMO_FS__
#define __WEMO_FS__


#define CONFIG_FILE "config.txt"
#define LOG_FILE    "log.txt"

uint8_t wemo_fs_init(void);
//char* wemo_fs_get_config(const char* key);
//void wemo_fs_set_config(const char* key, const char* value);
void wemo_write_config(void);
void wemo_read_config(void);
void wemo_log(const char* content);

#endif
