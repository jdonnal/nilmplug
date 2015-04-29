#!/usr/bin/python

import serial
import numpy as np
import matplotlib.pyplot as plt
import pdb

WEMO = "/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_DJ0057UG-if00-port0"
NUM_LINES = 100

wemo  = None

def read_data(wemo):
    #align the input
    match = 0
    while(True):
        ch = wemo.read(1)
        print("%02X"%ord(ch))
        if(ord(ch)==0xAE):
            match=1
            continue
        if(match==1):
            if(ord(ch)==0x1E):
                wemo.read(28)
                break
            else:
                match = 0
    #now we are aligned,
    #read in a chunk of data
    print "aligned!"
    raw = wemo.read(30*NUM_LINES)
    data = np.fromstring(raw,'>u1')
    data = data.reshape(NUM_LINES,30)
    #result is an array of the register values
    result = np.zeros([NUM_LINES,9])
    curline = 0
    for line in data:
        #print the line
        #print("".join(["%02X"%x for x in line]))
        #now verify the check sum
        #checksum is the 2's complement of 
        # the sum of all bytes modulo 256
        cs =0
        for x in line[:-1]:
            cs = (cs+x)%256
        cs = (~cs)+1
        cs = cs&0xFF
        if(cs!=line[-1]):
            print "checksum error: %02X != %02X"%(cs,line[-1])
        #now extract out the 9 3-byte values of the packet
        regs = []
        vals = line[2:29]
        for i in range(9):
            v1 = vals[3*i]; v2 = vals[1+3*i]; v3 = vals[2+3*i]
            val = v3<<16 | v2<<8 | v1
            regs.append(val)
        #just print the first reg
        result[curline,:] = regs
        curline+=1
        #print("%06X | %06X"%(regs[0],regs[1]))
    #plot the first reg
#    print result[:,0]
#    print result[:,1]
#    print result[:,2]
#    print result[:,3]
#    print result[:,4]
#    print result[:,5]
#    print result[:,6]
#    print result[:,7]
#    print result[:,8]

def wemo_write(wemo,cmd):
    pkt_len = len(cmd)+3
    pkt = [0xAA,pkt_len]
    pkt += cmd
    cs = checksum(pkt)
    print "".join("%02X"%x for x in pkt)
    for d in pkt:
        wemo.write(chr(d))
    wemo.write(chr(cs))

def read_command_reg(wemo):
    #select the device
    wemo_write(wemo,[0xCF,0x02])
    #set the target address
    wemo_write(wemo,[0xA3,0x00,0x00])
    #write the data
    wemo_write(wemo,[0xD1,0x00])
    #read the address
    wemo_write(wemo,[0xE1])
    #now see what happened
    read_data(wemo)

def checksum(data):
    #checksum is the 2's complement of 
    # the sum of all bytes modulo 256
    cs =0
    for x in data:
        cs = (cs+x)%256
    cs = (~cs)+1
    cs = cs&0xFF
    return cs
    
if __name__=="__main__":
    #open up the serial port
    wemo = serial.Serial(WEMO,9600)
    read_command_reg(wemo)
#    read_data(wemo)
