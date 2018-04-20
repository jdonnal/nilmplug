target extended-remote /dev/serial/by-id/usb-Black_Sphere_Technologies_Black_Magic_Probe_E3BCABBE-if00 
#/dev/serial/by-id/usb-Black_Sphere_Technologies_Black_Magic_Probe_E3BEABE0-if00 

mon swdp_scan

attach 1
file bin/flash.elf
load bin/flash.elf
#layout src

#b monitor.c:159


