import sys

LOCAL_IP = "0.0.0.0"
LOCAL_PORT = 20002
BUFFER_SIZE = 1024
COM_PORT=''
BAUD_RATE=9600
SHOW_UI='cmd' # cmd|ui|both

if len(sys.argv) < 2:
        print("Usage: {} COMPORT [LOCAL IP] [LOCAL PORT] [BAUDRATE] [Show UI (cmd|ui|both)]".format(sys.argv[0]))
        sys.exit(-1)

COM_PORT = sys.argv[1]
LOCAL_IP = sys.argv[2] if len(sys.argv) >2 else LOCAL_IP
LOCAL_PORT = int(sys.argv[3]) if len(sys.argv) >3 else LOCAL_PORT
BAUD_RATE = int(sys.argv[4]) if len(sys.argv) >4 else BAUD_RATE
SHOW_UI = sys.argv[5] if len(sys.argv) >5 else SHOW_UI


print("COM:{}@{} UDP:{}:{} UI:{}".format(COM_PORT, BAUD_RATE, LOCAL_IP, LOCAL_PORT, SHOW_UI))

import socket
import json
import serial
import time
if SHOW_UI in ["ui", "both"]:
        import tkinter
        from tkinter import *

import threading
import geopy
import geopy.distance
from math import pi,sqrt,asin,atan


FRASCA_WSG86_500POINT_LAT=64.931388
FRASCA_WSG86_500POINT_LON=25.375800

global point0
point0=geopy.Point(FRASCA_WSG86_500POINT_LAT,FRASCA_WSG86_500POINT_LON)




# One being constantly send via UDP
global datagram
datagram = {
	"HA": 200,
	"HKr": 1.13325746768,
	"HMr": 0.44254271807,
	"HCr": 0,
        "HEr": 0,
        "HGr": 0
}
# One containing all commands to Frasca simulator
global command_array
command_array = {
        "HA": "Altitude",
        "HM": "East position",
        "HK": "North position",
#        "HV": "Velocity",
        "HC": "Heading",
        "HE": "Pitch",
        "HG": "Bank",
#        "HQ": "Slip",
#        "HP": "Rot",
#        "HR": "Vertical speed"
}

# One containing lates raw-data from Frasca simulator
global data_array
data_array = {}
for k in command_array:
        data_array[k] = 0.0

class App(threading.Thread):

        def __init__(self):
                threading.Thread.__init__(self)
                self.start()

        def callback(self):
                self.root.quit()
        def update_data(self):
                global data_array
                for k in data_array:
                        self.sv_array[k].set(str(data_array[k]))
                self.root.after(100,self.update_data)
                
        def run(self):
                self.root = tkinter.Tk()
                self.root.protocol("WM_DELETE_WINDOW", self.callback)

                global command_array
                
                self.sv_array = {}
                c_row=0
                for k in command_array:
                        self.sv_array[k] = StringVar()
                        label = Label(self.root, text=command_array[k])
                        label.grid(row=c_row,column=0)
                        entry = Entry(self.root, font = ('arial', 18, 'bold'), textvariable = self.sv_array[k], width = 20, bg = "#eee", bd = 0, justify = RIGHT, state='readonly')
                        entry.grid(row=c_row,column=1)
                        c_row=c_row+1
                        
                self.root.after(100,self.update_data)
                self.root.mainloop()

class TerminalApp(threading.Thread):
        def __init__(self):
                threading.Thread.__init__(self)
                self.start()
        def run(self):
                global data_array
                while True:
                        sys.stdout.write(UDPSender.packdatagram(False,data_array).decode()+"\r")
                        sys.stdout.flush()
                        time.sleep(0.1)



class DataPoller(threading.Thread):

        def __init__(self, COM_PORT, BAUD_RATE):
                threading.Thread.__init__(self)
                self.ser = serial.Serial(COM_PORT, BAUD_RATE,timeout=1)
                self.start()
        def _readline(self):
                eol = b'\r'
                leneol = len(eol)
                line = bytearray()
                while True:
                                c = self.ser.read(1)
                                if c:
                                        line += c
                                        if line[-leneol:] == eol:
                                                break
                                else:
                                        break
                return bytes(line)
        def poll(self, command):
                self.ser.write((command+"\r").encode())
                answer = self._readline()
                answer_float = 0.0
                try:
                        answer_float = float(answer)
                except:
                        1+1
                return answer_float
        def convert_data_array_to_datagram(self):
                global data_array
                global datagram
                global point0
                # HM=East, HK=North
                datagram["HA"] = data_array["HA"]
                datagram["HCr"] = data_array["HC"]*(pi/180.0)
                datagram["HGr"] = data_array["HG"]*(pi/180.0)*-1.0
                datagram["HEr"] = data_array["HE"]*(pi/180.0)*-1.0
                dy=data_array["HK"]-500.0
                dx=data_array["HM"]-500.0
                dc=sqrt(pow(dy,2)+pow(dx,2))
                alpha=0 if dy == 0.0 else atan(abs(dx)/abs(dy))
                heading=(alpha if dy>0 and dx>=0
                         else
                         (pi-alpha if dy<=0 and dx>0
                          else
                          (alpha+pi if dy<0 and dx<=0
                           else
                           (2*pi-alpha if dy>=0 and dx<0
                            else -1))))

#                dist = geopy.distance.VincentyDistance(nautical=dc)
                dist = geopy.distance.geodesic(nautical=dc)
#                print(dy,dx)
#                print(dist,heading)
                point1 = dist.destination(point=point0,bearing=heading*(180.0/pi))

                datagram["HKr"] = point1.latitude*(pi/180.0)
                datagram["HMr"] = point1.longitude*(pi/180.0)

        def run(self):
                global data_array
                while True:
                        polling_sequence_started = time.time()
                        for k in data_array:
                                data_array[k]=self.poll(k)
                        self.convert_data_array_to_datagram()
                        #print("Polling rate",1.0/(time.time()-polling_sequence_started),"r/s")

class UDPSender(threading.Thread):
        def __init__(self, LOCAL_IP, LOCAL_PORT):
                threading.Thread.__init__(self)
                self.UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
                self.UDPServerSocket.bind((LOCAL_IP, LOCAL_PORT))
                print("UDP server up and listening")
                self.start()
                
        def packdatagram(self,dg):
                rv = ""
                for k in dg:
                       rv = rv + (" " if len(rv)>0 else "") + k + " "  + str(dg[k])
                return rv.encode()
        def run(self):
                global datagram
                while True:
                        bytesAddressPair = self.UDPServerSocket.recvfrom(BUFFER_SIZE)
                        message = bytesAddressPair[0].decode()
                        address = bytesAddressPair[1]
                        if message == 'OK':
                                self.UDPServerSocket.sendto(self.packdatagram(datagram), address)

data_poller = DataPoller(COM_PORT, BAUD_RATE)
udp_sender = UDPSender(LOCAL_IP, LOCAL_PORT)

if SHOW_UI in ["ui","both"]:
        app = App()
if SHOW_UI in ["cmd","both"]:
        app = TerminalApp()

