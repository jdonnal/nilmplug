#ifndef __MONITOR_H__
#define __MONITOR_H__

#include "server.h"

void monitor(void);

//board pins
#define RELAY_PIN PIO_PA13_IDX


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

void core_log_power_data(power_pkt *data);

#endif
