# Intro

This project aims to gather data from frasca bonanza simulator (late '70s/early '80s), convert it to correct format and input it to Flight Simulator X (might also work with Flight Simulator 2020)

Project consits of two applications (and debuggers) which communicate via UDP (port 20002, currently hardcoded)

# data_proxy.py

Main program to gather data from frasca simulator (serial port) and send it in standardized format via UDP 

It's a python script requiring following dependencies:
* pyserial
* geopy

It runs two threads:
* first one polls data from frasca simulator, converts it to right format and saves into global object 
* second one sends constantly data from global object via UDP to anyone asking via UDP (with message 'OK')

## Frasca serial protocol

documented in gdocs: https://docs.google.com/spreadsheets/d/1xU_mifn_-DgUhoRsJCmHYQb8DrCsCXLOC67u6_SZR88

## UDP protocol 

UDP protocol is very simple, max message length is 1024 and it consists
`<datalabel1> <data1> <datalabel2> <data2> <datalabel3> <data3>` ...

# Set Data.exe

Second program to read UDP data and send it to FSX using SimConnect: Program listens to port 20002 constantly (parameter 2), sends OK to it and waits 1sec max for resepose. After getting response program sends it directly to FSX using SimConnect 
Program runs in while-loop, respecting SimConnect simulation_end event.

Smoothing can currently only be configured by the timeframe where averages are being calculated.

@Todo: currently data from frasca is quite categorical, making Flight Simulator experiece quite "laggy". Better algorithm to smooth catecorigal data should be implemented into Set Data.exe. Current algorithm takes a timeframe (default 2000ms) and calculates averages for every parameter making it quite smooth. But responsiveness suffers when using averages >100ms.

@Todo: current implementation is more of a PoC, code should be refactored

Requires SimConnect SDK
* SimConnect.lib 
* simconnect.dll

Currently supported datalabels are
* HA = Altitude
* HKr = Latitude in radians
* HMr = Longitude in radians
* HEr = Pitch in radians
* HGr = Bank in radians 
* HCr = Heading in radians 

Usage: Set Data.exe [server IP] [server Port] [smoothing time frame = 2000]

# Debuggers

## location_debug.py

This tool aims to help configure frasca simulator locations according to fsx.

It consists of UI where you can input plane location and attitude data, it sends the data according to protocol to Set Data.exe. This way you can "simulate" any data sent that would be sent from frasca.

## simulate_data.py

Sends synthetic frasca data to COM-port. This is automatic simulation of frasca.

NOTICE! you'll need a virtual serial port adapter to bypass data from COM-port to another (for example com0com on windows)

## data_replay.py

Allows you to replay data recorded by frasca. 

Usage: just run `python3 data_replay.py`

data_replay.py has hardcoded ports, portrate and dataset to replay. In order to change these edit the .py file. @todo: prametrize data_replay.py

NOTICE!: There are currently no script to record data. It's a @todo: write script to write frasca data according to frasca-dataa/kuvaus.txt procotol



# Setup & running

* Install python 3.x
* Install geopy, pyserial modules (using pip)
* Connect serial port to computer
* Run `python3 data_proxy.py /dev/ttyUSB0` (or whatever is the COM port, see also other parameters of data_proxy.py if more config wanted) 
* Run Flight Simulator
* Run Set Data.exe on FSX computer `"Set Data.exe" 127.0.0.1 20002` (check ip and ports if data_proxy is being run somewhere else)
* Fly with Frasca, see world in Flight Simulator!


