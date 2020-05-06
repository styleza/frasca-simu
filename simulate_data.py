import socket
import threading
import time
import serial

COM_PORT='COM2'
BAUD_RATE=9600

global data_array
data_array = {
    'HK': 64.931388,
    'HM': 25.375800,
    'HA': 200,
    'HC': 0,
    'HE': 0,
    'HG': 0
}

class DataSender(threading.Thread):
    def __init__(self, COM_PORT, BAUD_RATE):
        threading.Thread.__init__(self)
        self.ser = serial.Serial(COM_PORT, BAUD_RATE)
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
        while True:
            ask = self._readline().decode().strip()
            if ask in data_array:
                self.ser.write((str(data_array[ask])+"\r").encode())

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
            data_array['HC'] = data_array['HC']+0.036
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
