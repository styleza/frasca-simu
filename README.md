# Intro

This project aims to gather data from frasca bonanza simulator (late '70s/early '80s), convert it to correct format and input it to Flight Simulator X

Project consits of two applications which communicate via UDP (port 20002, currently hardcoded)

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

Second program to read UDP data and send it to FSX using SimConnect: Program listens to port 20002 constantly, sends OK to it and waits 1sec max for resepose. After getting response program sends it directly to FSX using SimConnect 
Program runs in while-loop, respecting FSX simulation_end event



Requires Flight Simulator X SDK
* SimConnect.lib 
* simconnect.dll

Currently supported datalabels are
* HA = Altitude
* HKr = Latitude in radians
* HMr = Longitude in radians
* HEr = Pitch in radians
* HGr = Bank in radians 
* HCr = Heading in radians 



