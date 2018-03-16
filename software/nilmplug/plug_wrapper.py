#!/usr/bin/python

desc = """nilm-plug

    Command Line interaction with NILM Smart Plug
    
    Usage:

    1.) Control plug relay:
        nilm-plug --relay on  192.168.1.4
        nilm-plug --relay off 192.168.1.4
    2.) Read meter over WiFi, appending to a data file
        nilm-plug --read -f meter.dat 192.168.1.4
    3.) Download data over USB, appending to a data file
        nilm-plug --read -f meter.dat /dev/ttyACM0
    4.) Open command line interface with USB connected plug
        nilm-plug --cli #/dev/NODE is optional when only one plug connected
        nilm-plug --cli /dev/ttyACM0 #specify /dev/NODE for multiple plugs

    Data files created by this script are CSV formatted with the following columns
           ts | vrms | irms | watts | pavg | pf | freq | kwh 

    ts   | timestamp (UNIX milliseconds)
    vrms | RMS Voltage
    irms | RMS Current
    watts| Watts
    pavg | 30 second average of watts
    pf   | Power Factor
    freq | Line Frequency (Hz)
    kwh  | Energy used since plugged in (kWh)

    John Donnal 2015
------------------------------------------------------------------------------

"""

import socket
import os
import argparse
from . import initialize
import csv
from .plug import Plug
from . import terminal

FNULL = open(os.devnull,'w')

def set_relay(device,value,usb):
    plg = Plug(device,usb)
    plg.set_relay(value)

def read_meter(device,dest_file,usb,erase=False):
    plg = Plug(device,usb)
    last_ts = 0
    #open the destination file and read the last timestamp
    if(os.path.isfile(dest_file)):
        with open(dest_file) as f:
            reader = csv.reader(f)
            for row in reader:
                last_ts = row[0]
    #check the plug for new data
    data = plg.get_data(int(float(last_ts)))
    if(data==None):
        print("no new data available, exiting")
    else:
        #if we are on wifi, append the data to the file
        if(usb==False):
            with open(dest_file, 'a') as f:
                writer = csv.writer(f)
                writer.writerows(data)
        #if this is a USB data dump, don't append
        else:
            with open(dest_file,'w') as f:
                writer = csv.writer(f)
                writer.writerows(data)
            #erase if requested
            if(erase):
                plg.erase_data()
                print("\t erased data")
            print("All data retrieved, unplug smartee to reset")
def main():
        parser = argparse.ArgumentParser(
            formatter_class = argparse.RawDescriptionHelpFormatter,
            description = desc)
 
        group = parser.add_mutually_exclusive_group(required=True)
        group.add_argument("--relay", action="store", choices=["on","off"],
                           help="Set relay state")
        group.add_argument("--read", action="store_true",
                            help="request meter data")
        group.add_argument("--erase", action="store_true", 
                            help="erase data after reading (USB only)")
        group.add_argument("--cli",action="store_true",
                           help = "open plug command line interface (USB only)")
        group.add_argument("--initialize", action="store_true",
                            help = "install nilm-plug udev rules (requires root)")
        parser.add_argument("--file",action="store",default="plug.dat",
                            help="destination file for meter data")
        parser.add_argument("device", action="store", default="/dev/smart_plug",
                            nargs='?',
                            help="Device: either a /dev/NODE or an IPv4 address")
        
        args = parser.parse_args()

        #if initialize is specified run the setup routine and exit
        if(args.initialize):
            initialize.run()
            exit(0)
            
        #check if the device looks like an IP address
        try:
            socket.inet_aton(args.device)
            usb=False
        except socket.error:
            #not an IP address, see if its a device node
            if(os.path.exists(args.device)):
                usb = True
            else:
                #if the default isn't found display custom error
                if(args.device=="/dev/smart_plug"):
                    print("Error: no smart plug found, specify IP address or /dev/NODE")
                else:
                    print("Error: device [%s] not found"%args.device)
                exit(1)
        #make sure erase is only used if [usb] and [read] are specified
        if((args.erase or args.cli) and (not usb)):
            print("Error: cannot do this over wifi, connect with USB")

        #-----basic validation checks out, perform the requested action---
        if(args.relay):
            set_relay(args.device,args.relay,usb=usb)
        elif(args.read):
            read_meter(args.device,args.file,usb=usb,erase=False)
        elif(args.erase):
            read_meter(args.device,FNULL,usb=True,erase=True)
        elif(args.cli):
            terminal.main(args.device)
        else:
            print("Error: no action specified (shouldn't get here...)")
            exit(1)
        
        exit(0)

if __name__ == "__main__":
    main()
