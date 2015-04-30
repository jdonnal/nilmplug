#ifndef __MONITOR_H__
#define __MONITOR_H__

void monitor(void);

//board pins
#define RELAY_PIN PIO_PA13_IDX


// Functions implementing monitor commands.
int mon_test(int argc, char **argv);
int mon_set_rtc(int argc, char **argv);
int mon_get_rtc(int argc, char **argv);
int mon_relay_on(int argc, char **argv);
int mon_relay_off(int argc, char **argv);
int mon_relay_toggle(int argc, char **argv);

#endif
