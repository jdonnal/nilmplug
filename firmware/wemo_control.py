#!/usr/bin/python

import serial
import time
import datetime
import pdb

DEV_WEMO = "/dev/tty.usbmodemfa133431"
ser_wemo = None

def initialize():
    global ser_wemo
    ser_wemo = serial.Serial(DEV_WEMO,115200,timeout=None)
    
def get_time():
    ser_wemo.write("get_rtc\n")
    val=ser_wemo.readline()
    print(val)
    return int(val)

def set_time():
    #convert the time to RTC register format
    now = datetime.datetime.fromtimestamp(time.time())
    yr = now.year - 2000
    mo = now.month
    dt = now.day
    dw = now.weekday()
    hr = now.hour
    mn = now.minute
    sc = now.second
    ser_wemo.write("set_rtc %d %d %d %d %d %d %d\n"%
                   (yr,mo,dt,dw,hr,mn,sc))
    print("set WEMO clock to %s"%now.ctime())

def main():
    initialize()
    set_time()
    while(1):
        print(ser_wemo.readline());
#        val = get_time()
#        print("WEMO time: %d"%val)
#        time.sleep(1)
    
if __name__=="__main__":
    main()
