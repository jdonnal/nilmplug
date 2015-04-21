#ifndef __WEMO_FS__
#define __WEMO_FS__


#define CONFIG_FILE "config.txt"
#define LOG_FILE    "0:log.txt"

extern struct config wemo_config;

uint8_t wemo_fs_init(void);
//char* wemo_fs_get_config(const char* key);
//void wemo_fs_set_config(const char* key, const char* value);
void wemo_write_config(void);
void wemo_read_config(void);
void wemo_log(const char* content);

extern struct config wemo_config;

#endif
