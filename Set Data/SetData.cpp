//------------------------------------------------------------------------------
//
//  SimConnect Set Data Sample
// 
//	Description:
//				When ctrl-shift-A is pressed, the user aircraft is moved
//				to a new location
//------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>

#define _USE_MATH_DEFINES
#include <math.h>

#include "SimConnect.h"
#include "Network.h"


int     quit = 0;
HANDLE  hSimConnect = NULL;

static enum GROUP_ID {
    GROUP_6,
};

static enum INPUT_ID {
    INPUT_6,
};

static enum EVENT_ID{
    EVENT_SIM_START,
    EVENT_6,
    EVENT_1SEC,
    KEY_FREEZE_LATITUDE_LONGITUDE_SET,
    KEY_FREEZE_ALTITUDE_SET,
    KEY_FREEZE_ATTITUDE_SET,
};

static enum DATA_DEFINE_ID {
    PLANE_ALTITUDE,
    PLANE_LAT,
    PLANE_LON,
    PLANE_PITCH,
    PLANE_BANK,
    PLANE_HEADING
};

static enum DATA_REQUEST_ID {
    REQUEST_6,
};

WSASession Session;
UDPSocket Socket;
char UDPBuffer[1024];

void CALLBACK MyDispatchProcSD(SIMCONNECT_RECV* pData, DWORD cbData, void *pContext)
{
    HRESULT hr;
    
    switch(pData->dwID)
    {
       case SIMCONNECT_RECV_ID_EVENT:
        {
            SIMCONNECT_RECV_EVENT *evt = (SIMCONNECT_RECV_EVENT*)pData;

            switch(evt->uEventID)
            {
				case EVENT_SIM_START:
                    {
                    hr = SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, KEY_FREEZE_LATITUDE_LONGITUDE_SET, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                    hr = SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, KEY_FREEZE_ALTITUDE_SET, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                    hr = SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, KEY_FREEZE_ATTITUDE_SET, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                   
                    }
                    break;

                default:
                    printf("\nReceived2:%d", pData->dwID);
                    break;
            }
            break;
        }
        
        case SIMCONNECT_RECV_ID_QUIT:
        {
            quit = 1;
            break;
        }

        default:
            printf("\nReceived:%d",pData->dwID);
            break;
    }
}


bool initFSX()
{
    HRESULT hr;

    if (SUCCEEDED(SimConnect_Open(&hSimConnect, "Set Data", NULL, 0, 0, 0)))
    {
        printf("\nConnected to Flight Simulator!");   
        
        // Set up a data definition for positioning data
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_ALTITUDE, "Plane Altitude", "feet");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_LON, "Plane LONGITUDE", "radians");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_LAT, "Plane LATITUDE", "radians");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_BANK, "PLANE BANK DEGREES", "radians");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_PITCH, "PLANE PITCH DEGREES", "radians");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_HEADING, "PLANE HEADING DEGREES MAGNETIC", "radians");
        printf("\ndd:%d", hr);

        hr = SimConnect_MapClientEventToSimEvent(hSimConnect, KEY_FREEZE_LATITUDE_LONGITUDE_SET, "FREEZE_LATITUDE_LONGITUDE_SET");
        hr = SimConnect_MapClientEventToSimEvent(hSimConnect, KEY_FREEZE_ALTITUDE_SET, "FREEZE_ALTITUDE_SET");
        hr = SimConnect_MapClientEventToSimEvent(hSimConnect, KEY_FREEZE_ATTITUDE_SET, "FREEZE_ATTITUDE_SET");

        hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart");

     
        return true;
    }
    else {
        printf("Connection to FSX not initiated...");
        return false;
    }
}

// Helper function to split strings by any char
size_t split(const std::string& txt, std::vector<std::string>& strs, char ch)
{
    size_t pos = txt.find(ch);
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while (pos != std::string::npos) {
        strs.push_back(txt.substr(initialPos, pos - initialPos));
        initialPos = pos + 1;

        pos = txt.find(ch, initialPos);
    }

    // Add the last one
    strs.push_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));

    return strs.size();
}

void parseVarsToMap(const std::vector<std::string>& v, std::map<std::string, double>& m) {
    m.clear();
    for (int i = 0; i < v.size(); i++) {
        // get every second from vector parse it into double and add to map with key as previous value from vector
        if (i % 2 == 1) {
            double val = std::stod(v[i]);
            m[v[i - 1]]= val;
        }
    }
}

void sendDataToFSX(double& Altitude, double& Latitude, double& Longitude, double& Pitch, double& Bank, double& Heading) {
    HRESULT hr;
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_ALTITUDE, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(double), &Altitude);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_LAT, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(double), &Latitude);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_LON, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(double), &Longitude);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_PITCH, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(double), &Pitch);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_BANK, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(double), &Bank);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_HEADING, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(double), &Heading);
}

void eventloop(const char* server, const char* port, LONG timeDelta) {
    // Request newest data from UDP server
    std::string data = "OK";
    std::vector<std::string> v;
    std::map<std::string, double> m;


    Socket.SendTo(server, atoi(port), data.c_str(), data.size());
    Socket.RecvFrom(UDPBuffer, 1024);

    // Split data by ' '
    split(UDPBuffer, v, ' ');

    // parse vector into map (decode dataprotocol)
    parseVarsToMap(v, m);

    // Send data to simulator
    printf("Altitude: %f, Lat: %f, Lon: %f, Pitch: %f, Bank: %f, Heading: %f, TDELTA: %ld \r", m["HA"], m["HKr"], m["HMr"], m["HEr"], m["HGr"], m["HCr"], timeDelta);
    sendDataToFSX(m["HA"], m["HKr"], m["HMr"], m["HEr"], m["HGr"], m["HCr"]);


}

int main(int argc, const char* argv[])
{
    if (!initFSX() || argc < 3) {
        return -1;
    }
    SYSTEMTIME time;
    GetSystemTime(&time);
    LONG last_time = time.wMilliseconds;
    while (quit == 0) {
        GetSystemTime(&time);
        LONG delta =  time.wMilliseconds - last_time;
        last_time = time.wMilliseconds;
        eventloop(argv[1], argv[2],delta);
        SimConnect_CallDispatch(hSimConnect, MyDispatchProcSD, NULL);
        Sleep(1);
    }
    SimConnect_Close(hSimConnect);

	return 0;
}