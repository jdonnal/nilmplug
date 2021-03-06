import socket
import numpy as np
from calendar import timegm
import time
import serial
import datetime
import asyncio


class Plug:
    # structure of packet
    NUM_SAMPLES = 30
    STATUS_SIZE = 4
    TIMESTAMP_SIZE = 20
    TCP_PKT_SIZE = (7 * NUM_SAMPLES * 4) + STATUS_SIZE + TIMESTAMP_SIZE
    SECONDS_BTWN_SAMPLES = 2
    SECONDS_BTWN_SAMPLES = 2
    PLUG_PORT_NUMBER = 1336

    def __init__(self, device, usb=False):
        # [device] should be an IPv4 address for WiFi or
        #         a device node if plug is connected by USB (eg /dev/ttyACM0)
        self.device = device
        self.usb = usb

    ######### Relay Management #############
    def set_relay(self, value):
        print("set relay [%s]" % value)
        if (self.usb):
            return self.__set_relay_usb(value)
        else:
            return self.__set_relay_wifi(value)

    def __set_relay_usb(self, value):
        if (value != "on" and value != "off"):
            print("value must be [on|off]")
            return
        dev = serial.Serial(self.device)
        time.sleep(1.5)  # wait for welcome message
        dev.write("echo off\n")
        time.sleep(0.5)
        dev.flushInput()
        time.sleep(0.5)
        dev.write("relay %s\n" % value)  # LED solid green
        time.sleep(0.5)
        dev.close()

    def __set_relay_wifi(self, value):
        # now open up a connection to the plug
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            s.connect((self.device, Plug.PLUG_PORT_NUMBER))
        except socket.error:
            print("error, can't connect to plug")
            return -1
        except Exception as e:
            print("error: other exception")
            print(e)
            return -1
        if (value == "on"):
            s.sendall('relay_on'.encode('ascii'))
        elif (value == "off"):
            s.sendall('relay_off'.encode('ascii'))
        s.settimeout(3.0)
        try:
            resp = s.recv(2).decode('ascii')
        except socket.timeout:
            print("error, no response from plug")
            return -1
        except Exception as e:
            print("error: other exception")
            print(e)
            return -1
        print("closing socket")
        s.close()
        if resp == "OK":
            return 0
        else:
            print(("bad response: %s", resp))
            return -1

    ######### Calibration Mode ###############
    def set_calibrate(self, run, on_time=0, off_time=0):
        if not self.usb:
            print("error: cannot change calibration mode over WiFi")
            return
        if run and (on_time < 1000 or off_time < 1000):
            print("error: invalid settings for on and off times")
            return
        dev = serial.Serial(self.device)
        time.sleep(1.5)  # wait for welcome message
        dev.write("echo off\n")
        time.sleep(0.5)
        dev.flushInput()
        time.sleep(0.5)
        if (run):
            dev.write("calibrate start %d %d\n" % (on_time, off_time))
        else:
            dev.write("calibrate stop\n")
        time.sleep(0.5)
        dev.close()

    ######### RTC Management     #############
    async def set_rtc(self):
        utc = datetime.datetime.utcnow()
        yr = int(utc.strftime("%y"))
        mo = utc.month
        dt = utc.day
        dw = int(utc.strftime("%w"))
        hr = utc.hour
        mn = utc.minute
        sc = utc.second

        if (self.usb):
            return self.__set_rtc_usb(yr, mo, dt, dw, hr, mn, sc)
        else:
            return await self.__set_rtc_wifi(yr, mo, dt, dw, hr, mn, sc)

    async def __set_rtc_usb(self, yr, mo, dt, dw, hr, mn, sc):
        dev = serial.Serial(self.device)
        await asyncio.sleep(1.5)  # wait for welcome message
        dev.write("echo off\n")
        time.sleep(0.5)
        dev.flushInput()
        time.sleep(0.5)
        dev.write("rtc  set %d %d %d %d %d %d %d\n" %
                  (yr, mo, dt, dw, hr, mn, sc))
        time.sleep(0.5)
        dev.close()

    async def __set_rtc_wifi(self, yr, mo, dt, dw, hr, mn, sc):
        # now open up a connection to the plug
        try:
            reader, writer = await asyncio.open_connection(self.device, Plug.PLUG_PORT_NUMBER)
            msg = ('set_rtc_%d_%d_%d_%d_%d_%d_%d.' % (yr, mo, dt, dw, hr, mn, sc))
            writer.write(msg.encode())
            await writer.drain()
            resp = await reader.read(100)
            if resp.decode() == "OK":
                return 0
            else:
                print("bad response: ", resp.decode())
            writer.close()
            await writer.wait_closed()
        except socket.error:
            print("error, can't connect to plug")
            return -1
        except Exception as e:
            print("error: other exception")
            print(e)
            return -1

    ######### RGB LED Management #############
    def set_led(self, red, green, blue, blink):
        if (self.usb):
            return self.__set_led_usb(red, green, blue, blink)
        else:
            return self.__set_led_wifi(red, green, blue, blink)

    def __set_led_usb(self, red, green, blue, blink):
        dev = serial.Serial(self.device)
        time.sleep(1.5)  # wait for welcome message
        dev.write("echo off\n")
        time.sleep(0.5)
        dev.flushInput()
        time.sleep(0.5)
        dev.write("led %d %d %d %d\n" % (red, green, blue, blink))  # LED solid green
        time.sleep(0.5)
        dev.close()

    def __set_led_wifi(self, red, green, blue, blink):
        # now open up a connection to the plug
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            s.connect((self.device, Plug.PLUG_PORT_NUMBER))
        except socket.error:
            print("error, can't connect to plug")
            return -1
        except Exception as e:
            print("error: other exception")
            print(e)
            return -1

        s.sendall(('set_led_%d_%d_%d_%d.' % (red, green, blue, blink)).encode('ascii'))
        s.settimeout(3.0)
        try:
            resp = s.recv(2).decode('ascii')
        except socket.timeout:
            print("error, no response from plug")
            return -1
        except Exception as e:
            print("error: other exception")
            print(e)
            return -1
        print("closing socket")
        s.close()

        if resp == "OK":
            return 0
        else:
            print(("bad response: %s", resp))
            return -1

    ######### Meter reading ##############
    async def get_data(self, last_ts):
        if self.usb:
            return self.__get_data_usb()
        else:
            return await self.__get_data_wifi(last_ts)

    def __get_data_usb(self):
        dev = serial.Serial(self.device)
        time.sleep(1.5)  # wait for welcome message
        dev.write("echo off\n")
        time.sleep(0.5)
        dev.flushInput()
        # read the plug serial number
        dev.write("config get serial_number\n")
        time.sleep(0.5)
        serial_number = dev.readline().rstrip();
        print(("\tConnected to SmartEE [%s]" % serial_number))
        print("\t starting data dump")
        dev.write("led 255 255 0 500\n")  # LED orange blink
        time.sleep(0.5)
        dev.write("data read\n")  # start data download
        resp = dev.read(Plug.TCP_PKT_SIZE)
        if resp == 'x' * Plug.TCP_PKT_SIZE:
            time.sleep(0.5)
            dev.write("led 0 255 0 0\n")  # LED solid green
            time.sleep(0.5)
            dev.close()
            return None  # no data to download
        # remove overlapping data from this stream
        data = self.__parse_data(resp, 0)

        last_ts = data[-1][0]
        while resp != "x" * Plug.TCP_PKT_SIZE:
            # parse plug data into a numpy array
            res = self.__parse_data(resp, last_ts)
            if res != None:
                data = np.concatenate((data, res))
            resp = dev.read(Plug.TCP_PKT_SIZE)

        time.sleep(0.5)
        dev.write("led 0 255 0 0\n")  # LED solid green
        time.sleep(0.5)
        dev.close()
        return data

    async def __get_data_wifi(self, last_ts):
        # now open up a connection to the plug
        try:
            reader, writer = await asyncio.open_connection(self.device, Plug.PLUG_PORT_NUMBER)
            writer.write("send_data".encode())
            await writer.drain()
            resp = await reader.read(Plug.TCP_PKT_SIZE)
            writer.close()
            await writer.wait_closed()
            if resp == "error: no data".encode():
                return None
            print("read %d bytes of data"%len(resp))
            if len(resp) == 0:
                return None

            return self.__parse_data(resp, last_ts)
        except socket.timeout:
            print("error, no response from plug")
            return None
        except socket.error:
            print("error, can't connect to plug")
            return None
        except Exception as e:
            print("error: other exception")
            print(e)
            return None

    def erase_data(self):
        if not self.usb:
            print("error: cannot delete data over WiFi")
            return
        else:
            dev = serial.Serial(self.device)
            time.sleep(1.5)  # wait for welcome message
            dev.write("echo off\n")
            time.sleep(0.5)
            dev.flushInput()
            time.sleep(0.5)
            dev.write("data erase\n")
            dev.write("led 0 255 0 0\n")  # LED solid green
            time.sleep(0.5)
            dev.close()

    def __parse_data(self, resp, last_ts):
        NUM_SAMPLES = Plug.NUM_SAMPLES
        # now convert the response into a data object
        data = {}
        i = 0
        data["vrms"] = np.frombuffer(resp[i:i + 4 * NUM_SAMPLES], dtype=np.int32)
        i += 4 * NUM_SAMPLES
        data["irms"] = np.frombuffer(resp[i:i + 4 * NUM_SAMPLES], dtype=np.int32)
        i += 4 * NUM_SAMPLES
        data["watts"] = np.frombuffer(resp[i:i + 4 * NUM_SAMPLES], dtype=np.int32)
        i += 4 * NUM_SAMPLES
        data["pavg"] = np.frombuffer(resp[i:i + 4 * NUM_SAMPLES], dtype=np.int32)
        i += 4 * NUM_SAMPLES
        data["pf"] = np.frombuffer(resp[i:i + 4 * NUM_SAMPLES], dtype=np.int32)
        i += 4 * NUM_SAMPLES
        data["freq"] = np.frombuffer(resp[i:i + 4 * NUM_SAMPLES], dtype=np.int32)
        i += 4 * NUM_SAMPLES
        data["kwh"] = np.frombuffer(resp[i:i + 4 * NUM_SAMPLES], dtype=np.int32)
        i += 4 * NUM_SAMPLES
        data["time"] = resp[i:i + 19].decode('ascii')
        # create a UNIX timestamp from the date info (all plugs run UTC)
        try:
            utc_ts = timegm(time.strptime(data['time'], "%Y-%m-%d %H:%M:%S"))
        except ValueError:
            print("corrupt date stamp:", resp)
            return None
        # convert the data to proper units
        vrms = [float(x) / 1000.0 for x in data['vrms']]
        irms = [float(x) * 7.77e-6 for x in data['irms']]
        watts = [float(x) / 200.0 for x in data['watts']]
        pavg = [float(x) / 200.0 for x in data['pavg']]
        pf = [float(x) / 1000 for x in data['pf']]
        freq = [float(x) / 1000 for x in data['freq']]
        kwh = [float(x) / 1000 for x in data['kwh']]
        print(data['time'])
        # create the numpy array to put in nilmdb
        data_size = len(vrms)
        ts_start = int(round(utc_ts * 1e6))
        ts = [ts_start + Plug.SECONDS_BTWN_SAMPLES * 1e6 * i for i in range(data_size)]
        db_data = np.vstack([ts, vrms, irms, watts, pavg, pf, freq, kwh]).transpose()
        # make sure this new data doesn't overlap with existing data (due to drift in the plug clock)
        overlap = 0
        while ts[overlap] <= last_ts:
            overlap += 1
            if overlap >= len(ts):
                print("this data is all too old, ignoring")
                break
        db_data = db_data[overlap:]
        if len(db_data) == 0:
            return None  # total overlap, no new data here :(

        return db_data


# shorthand for getting a plug connected over USB
usb_plug = Plug("/dev/smart_plug", usb=True)

if __name__ == "__main__":
    plug = Plug("172.16.1.173")
    asyncio.run(plug.set_rtc())
    data = asyncio.run(plug.get_data(0))
    print(data)
