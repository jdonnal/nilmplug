#!/usr/bin/python

"""
Reflash the wemo image using sam-ba bootloader
**requires 99-atmel_samba.rules in udev**
**requires samba_64**
John Donnal
"""

import serial
import subprocess

BOARD="at91sam4s4-wemo"
WEMO_DEVICE="/dev/serial/by-id/usb-MIT_WEMO_Control_Board-if00"
SAMBA_DEVICE="/dev/serial/by-type/sam-ba"
HEX_FILE="../bin/flash.bin"
SAMBA_EXEC="/home/jdonnal/sam-ba_cdc_linux/sam-ba_64"
SAMBA_TCL="/home/jdonnal/wemo_board/scripts/samba_reflash.tcl"

print("##### WEMO Reflash Tool #####")
s = serial.Serial(WEMO_DEVICE)
print("\t entering bootloader")
s.write("restart bootloader\n")
s.close()
print("\t reflashing chip")
#sam-ba_64 /dev/ttyACM2 at91sam4s4-wemo usr/historyCommand.tcl
r = subprocess.call([SAMBA_EXEC,SAMBA_DEVICE,BOARD,SAMBA_TCL])
if(r==0):
    print("all done!")
else:
    print("Error [%d]"%r)

