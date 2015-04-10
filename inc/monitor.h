#ifndef __MONITOR_H__
#define __MONITOR_H__

void monitor(void);


// Functions implementing monitor commands.
int mon_test(int argc, char **argv);
int mon_set_rtc(int argc, char **argv);
int mon_get_rtc(int argc, char **argv);

#endif
