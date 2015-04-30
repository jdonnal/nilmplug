#!/usr/bin/python

import serial
import numpy as np

PORT = "/dev/tty.usbserial-DJ0057UG"
def main():
    #open up the serial port
    wemo = serial.Serial(PORT,9600)
    #realign to a packet (0xAE 0x1E)
    match=0
    while(True):
        ch = wemo.read(1)
        print("Got %X"%ord(ch))
        if(match==0):
            if(ord(ch)==0xAE):
                match=1
                continue
        if(match==1):
            if(ord(ch)==0x1E):
                wemo.read(28) 
                break #aligned
            else:
                match=0
    print "Aligned!"
    i=0
    rows = 50
    raw = wemo.read(30*rows)
    #parse from string: 1 byte units in big endian
    data = np.fromstring(raw,dtype=np.dtype('>u1'))
    rows  = len(raw)/30
    #give the array the proper shape
    data.shape = (rows,30)
    for i in range(rows):
        print "".join(["%02X"%x for x in data[i,:]])
    wemo.close()

if __name__ == "__main__":
    main()
