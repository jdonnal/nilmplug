#ifndef __MONITOR_H__
#define __MONITOR_H__

#include "wemo.h"

void monitor(void);

//board pins
#define RELAY_PIN  PIO_PA13_IDX
#define BUTTON_PIN PIO_PB14_IDX
#define VBUS_PIN   PIO_PB13_IDX

//Wemo in-memory config datastructure
//Read from file system on boot
typedef struct config_struct {
  char wifi_ssid[20];     //the wireless network to connect to
  char wifi_pwd[30];      //the network password (may be blank)
  uint8_t standalone;     //TRUE: don't try to connect to a network
  char mgr_url[30];       //URL of managment node (www.wattsworth.net)
  char wattsworth_id[30]; //ID wattsworth owner (nilm9F59)
  char wattsworth_ip[30]; //cached IP address of wattsworth owner
  char wemo_ip[30];       //currently assigned IP address
  uint8_t echo;           //runtime config, echo USB chars
} config;

extern config wemo_config;

// Functions implementing monitor commands
// these functions are meant to be called over
// serial, they are just wrappers for core commands
// firmware modules call the core commands directly
int mon_test(int argc, char **argv);
int mon_set_rtc(int argc, char **argv);
int mon_get_rtc(int argc, char **argv);
int mon_relay_on(int argc, char **argv);
int mon_relay_off(int argc, char **argv);
int mon_relay_toggle(int argc, char **argv);

//Functions implementing core commands
#define WIFI_RX_BUF_SIZE 300
char wifi_rx_buf [WIFI_RX_BUF_SIZE];
//   Incoming data from WiFi
void core_process_wifi_data(void);
void core_wifi_link(void);
void core_wifi_unlink(void);
//   Outgoing data to WiFi
void core_log_power_data(power_sample *data);
void core_transmit_power_packets(void);
//   System logging
void core_log(const char* content);
//   USB terminal functions
void core_read_usb(uint8_t port);
void core_usb_enable(uint8_t port, bool b_enable);

#endif
