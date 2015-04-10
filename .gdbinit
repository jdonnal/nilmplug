
target extended-remote /dev/ttyACM0
mon swdp_scan
attach 1
mon flash_erase_all
file bin/flash.elf
load bin/flash.elf
#b iv_adc.c:153
