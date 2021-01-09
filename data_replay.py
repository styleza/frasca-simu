import socket
import threading
import time
import serial
from datetime import datetime
import frasca
from math import pi

REPLAY_FILE='./frasca-dataa/dataset1.data'
COM_PORT='COM4'
BAUD_RATE=9600
BUFFER_SIZE = 1024

LOCAL_IP = "0.0.0.0"
LOCAL_PORT = 20002

global data_array
global round_data
global datagram
data_array = {
#    'HK': 487.4,
#    'HM': 223.6,
    'HK': 495.0,
    'HM': 400.0,
    'HA': 400,
    'HC': 0,
    'HE': 0,
    'HG': 0
}
datagram = {}

round_data = ['HC','HE','HG','HA']

class DataSender(threading.Thread):
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
    
    def run(self):
        global data_array
        global round_data
        while True:
            ask = self._readline().decode().strip()
            if ask in data_array:
                data = str(round(data_array[ask]) if ask in round_data else data_array[ask])
                self.ser.write((data+"\r").encode())

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

class DataReplay(threading.Thread):
        def __init__(self, REPLAY_FILE):
            threading.Thread.__init__(self)
            self.replay_data = open(REPLAY_FILE,'r').readlines()
            self.parse_data()
            self.start()
        def parse_data(self):
            print("starting to parse data...")
            zero_time = False
            self.data = []
            for l in self.replay_data:
                row = l.split(" ")
                try:
                    ts = datetime.strptime(row[0],"%H:%M:%S.%f:HA")
                    if not zero_time:
                        zero_time = ts
                    tdiff = (ts - zero_time).total_seconds()
                    self.data.append([tdiff,row[1],row[3],row[5],row[7],row[9],row[11]])
                except Exception as e:
                    print(e)
            print("Done parsing data!")

        def update_data(self):
            global data_array
            # check if next data is good already, otherwise wait. Unless we're lagging behind, then fast forward
            next_data = self.data[self.i]
            compare_ts = (datetime.now() - self.zero_time).total_seconds()
            # next data is in future, do nothing
            if next_data[0] > compare_ts:
                return
            # if we're in "good zone" update data, tolarance 0.05 sec
            if compare_ts - next_data[0] <= 0.05:
                data_array['HA'] = float(next_data[1])
                data_array['HM'] = float(next_data[2])
                data_array['HK'] = float(next_data[3])
                data_array['HC'] = float(next_data[4])
                data_array['HE'] = float(next_data[5])
                data_array['HG'] = float(next_data[6])
                self.i = self.i + 1
            # we're lagging too much behind
            else:
                self.i = self.i + 1
                self.update_data()
        def convert_data_array_to_datagram(self):
                global data_array
                global datagram

                # HM=East, HK=North (other way around!??)
                datagram["HA"] = data_array["HA"]
                datagram["HCr"] = data_array["HC"]*(pi/180.0)
                datagram["HGr"] = data_array["HG"]*(pi/180.0)*-1.0
                datagram["HEr"] = data_array["HE"]*(pi/180.0)*-1.0				
                point1 = frasca.from_frasca(data_array["HK"], data_array["HM"])
				
                datagram["HKr"] = point1.latitude*(pi/180.0)
                datagram["HMr"] = point1.longitude*(pi/180.0)
                
        def run(self):
            self.zero_time = datetime.now()
            self.i = 0
            while True:
                self.update_data()
                self.convert_data_array_to_datagram()
                time.sleep(0.05)
                
#data_sender = DataSender(COM_PORT,BAUD_RATE)
data_updater = DataReplay(REPLAY_FILE)
udp_sender = UDPSender(LOCAL_IP, LOCAL_PORT)
