import socket
import threading
import time
import serial

COM_PORT='COM4'
BAUD_RATE=9600

global data_array
global round_data
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

class DataUpdater(threading.Thread):
        def __init__(self):
            threading.Thread.__init__(self)
            self.start()
        def update_data(self):
            global data_array
            # move plane to NE "one point"
            data_array['HK'] = data_array['HK']+0.0002777
            data_array['HM'] = data_array['HM']+0.0002777
            data_array['HA'] = data_array['HA']+0.1
            data_array['HC'] = data_array['HC']+0.152
            if data_array['HC'] >= 360.0:
                data_array['HC'] = data_array['HC']-360.0
            
        def run(self):
            last_time = time.time()
            while True:
                self.update_data()
                time_delta = time.time()-last_time
                if time_delta <= 0.1:
                    time.sleep(0.1-time_delta)
                last_time = time.time()

data_sender = DataSender(COM_PORT,BAUD_RATE)
data_updater = DataUpdater()
