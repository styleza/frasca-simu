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
#include <functional>
#include <numeric>


#define _USE_MATH_DEFINES
#include <math.h>

#include "SimConnect.h"
#include "Network.h"


// millisecond value for average calculation. 1000=smooth proxy values from 1s time frame.
const int minTurnSmoothing = 2000;
const int maxTurnSmoothing = 5000;

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

struct plane_data {
    long double heading;
    long double pitch;
    long double bank;
    long double lat;
    long double lon;
    long double altitude;
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
                    printf("Received sim start, sending freezes\r\n");
                    }
                    break;

                default:
                    printf("\nReceived event:%d", pData->dwID);
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
            printf("Received:%d\r\n",pData->dwID);
            break;
    }
}


bool initFSX()
{
    HRESULT hr;

    if (SUCCEEDED(SimConnect_Open(&hSimConnect, "Set Data", NULL, 0, 0, 0)))
    {
        printf("\nConnected to Flight Simulator!\r\n");   
        
        // Set up a data definition for positioning data
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_ALTITUDE, "Plane Altitude", "feet");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_LON, "Plane LONGITUDE", "radians");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_LAT, "Plane LATITUDE", "radians");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_BANK, "PLANE BANK DEGREES", "radians");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_PITCH, "PLANE PITCH DEGREES", "radians");
        hr = SimConnect_AddToDataDefinition(hSimConnect, PLANE_HEADING, "PLANE HEADING DEGREES MAGNETIC", "radians");
        printf("Registered data definitions to flight simulator\r\n");

        hr = SimConnect_MapClientEventToSimEvent(hSimConnect, KEY_FREEZE_LATITUDE_LONGITUDE_SET, "FREEZE_LATITUDE_LONGITUDE_SET");
        hr = SimConnect_MapClientEventToSimEvent(hSimConnect, KEY_FREEZE_ALTITUDE_SET, "FREEZE_ALTITUDE_SET");
        hr = SimConnect_MapClientEventToSimEvent(hSimConnect, KEY_FREEZE_ATTITUDE_SET, "FREEZE_ATTITUDE_SET");
        printf("Sent freeze commands\r\n");

        hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart");
        printf("Subscribed to events\r\n");
     
        return true;
    }
    else {
        printf("Connection to FSX not initiated...\r\n");
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

void parseVarsToMap(const std::vector<std::string>& v, std::map<std::string, long double>& m) {
    m.clear();
    for (int i = 0; i < v.size(); i++) {
        // get every second from vector parse it into double and add to map with key as previous value from vector
        if (i % 2 == 1) {
            double val = std::stold(v[i]);
            m[v[i - 1]]= val;
        }
    }
}

void sendDataToFSX(long double& Altitude, long double& Latitude, long double& Longitude, long double& Pitch,  long double& Bank, long double& Heading) {
    HRESULT hr;
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_ALTITUDE, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(long double), &Altitude);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_LAT, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(long double), &Latitude);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_LON, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(long double), &Longitude);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_PITCH, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(long double), &Pitch);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_BANK, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(long double), &Bank);
    hr = SimConnect_SetDataOnSimObject(hSimConnect, PLANE_HEADING, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(long double), &Heading);
}

long double smoothHelper(long double accVal, int breakPoint, int smoothingStart, int totalLength) {
    if (breakPoint > smoothingStart) {
        return accVal / (breakPoint > 0 ? totalLength - breakPoint : totalLength);
    }
    
    return accVal / (smoothingStart > 0 ? totalLength - smoothingStart : totalLength);
}

// in practice this function is average calculator for plane position vector. It also takes into account 360' turns that may occur
plane_data smooth(std::vector<plane_data> *p, int pitchSmoothingStart, int bankSmoothingStart, int headingSmoothingStart, int latSmoothingStart, int lonSmoothingStart, int altitudeSmoothingStart) {
    plane_data x;
    long double pitch = 0;
    long double bank = 0;
    long double heading = 0; 
    int heading_break = 0;
    int bank_break = 0;
    int pitch_break = 0;
    long double lat = 0;
    long double lon = 0;
    long double altitude = 0;
    for (int i = 0; i < p->size(); i++) {
        if (i > 0) {
            if (abs(p->at(i - 1).pitch - p->at(i).pitch) > M_PI) {
                pitch_break = i;
                pitch = 0;
            }
            if (abs(p->at(i - 1).heading - p->at(i).heading) > M_PI) {
                heading_break = i;
                heading = 0;
            }
            if (abs(p->at(i - 1).bank - p->at(i).bank) > M_PI) {
                bank_break = i;
                bank = 0;
            }
        }
        if (i >= pitchSmoothingStart) {
            pitch += p->at(i).pitch;
        }
        if (i >= bankSmoothingStart) {
            bank += p->at(i).bank;
        }
        if (i >= headingSmoothingStart) {
            heading += p->at(i).heading;
        }
        if (i >= latSmoothingStart) {
            lat += p->at(i).lat;
        }
        if (i >= lonSmoothingStart) {
            lon += p->at(i).lon;
        }
        if (i >= altitudeSmoothingStart) {
            altitude += p->at(i).altitude;
        }
    }
    x.pitch = smoothHelper(pitch, pitch_break, pitchSmoothingStart, p->size());
    x.bank = smoothHelper(bank, bank_break, bankSmoothingStart, p->size());
    x.heading = smoothHelper(heading, heading_break, headingSmoothingStart, p->size());
    x.lat = smoothHelper(lat, 0, latSmoothingStart, p->size());
    x.lon = smoothHelper(lon, 0, lonSmoothingStart, p->size());
    x.altitude = smoothHelper(altitude, 0, altitudeSmoothingStart, p->size());

    return x;
}

// hold data for turnSmoothing!
std::vector<int> t_x;
std::vector<plane_data> t_y;

plane_data dvToAttitude(const std::map<std::string, long double>* m) {
    plane_data x;
    x.bank = m->at("HEr");
    x.pitch = m->at("HGr");
    x.heading = m->at("HCr");
    x.lat = m->at("HKr");
    x.lon = m->at("HMr");
    x.altitude = m->at("HA");

    return x;
}

void updatePrediction(std::map<std::string, long double>* m, long double current_tx) {
    double last_tx = t_x.size() > 0 ? t_x.at(t_x.size() - 1) : 0;
    double delta_smoothing = last_tx - maxTurnSmoothing;
    double next_tx = last_tx + current_tx;
    if (delta_smoothing > 0) {
        next_tx -= delta_smoothing;
        // timeframe needs shift, we've overcome turn smoothing limit 
        // assume that t_x is high to low sorted already
        unsigned int i = 0;
        while (i < t_x.size()) {
            if (t_x.at(i) < delta_smoothing) {
                t_x.erase(t_x.begin());
                t_y.erase(t_y.begin());
            }
            else {
                t_x[i] = t_x[i] - delta_smoothing;
                i++;
            }
        }
    }

    plane_data currentAttitude = dvToAttitude(m);

    t_x.push_back(next_tx);
    t_y.push_back(currentAttitude);

    // backtrack to previous values to found smoothing frame per value
    bool all_found = false;
    int i = t_y.size() - 1;
    int pitch_smooth_start = 0;
    int bank_smooth_start = 0;
    int heading_smooth_start = 0;
    int lat_smooth_start = 0;
    int lon_smooth_start = 0;
    int altitude_smooth_start = 0;
    plane_data last_values = currentAttitude;
    int minTx = next_tx - minTurnSmoothing;


    // @TODO: tämä aiheuttaa tökkimistä, paremman lopputuloksen saa jos smoothaa vaan tarpeeksi. Esim 2000ms. Eli pitää kaikki *_start nollissa
    // funtion idea on leikata dynaamisesti aikaikkunaa niin että se ottaa kahden edellisen arvon framen mukaan (tai vähintään minTurnSmoothing ajan)
    while (!all_found && i > 0) {
        if (pitch_smooth_start == 0 && last_values.pitch != currentAttitude.pitch) {
            int ii = i;
            long double v = last_values.pitch;
            while (ii > 0 && (v == t_y.at(ii).pitch || t_x.at(ii) >= minTx)) {
                ii--;
            }
            pitch_smooth_start = ii;
        }
        if (bank_smooth_start == 0 && last_values.bank != currentAttitude.bank) {
            int ii = i;
            long double v = last_values.bank;
            while (ii > 0 && (v == t_y.at(ii).bank || t_x.at(ii) >= minTx)) {
                ii--;
            }
            bank_smooth_start = ii;
        }
        if (heading_smooth_start == 0 && last_values.heading != currentAttitude.heading) {
            int ii = i;
            long double v = last_values.heading;
            while (ii > 0 && (v == t_y.at(ii).heading || t_x.at(ii) >= minTx)) {
                ii--;
            }
            heading_smooth_start = ii;
        }
        if (lat_smooth_start == 0 && last_values.lat != currentAttitude.lat) {
            int ii = i;
            long double v = last_values.lat;
            while (ii > 0 && (v == t_y.at(ii).lat || t_x.at(ii) >= minTx)) {
                ii--;
            }
            lat_smooth_start = ii/2;
        }
        if (lon_smooth_start == 0 && last_values.lon != currentAttitude.lon) {
            int ii = i;
            long double v = last_values.lon;
            while (ii > 0 && (v == t_y.at(ii).lon || t_x.at(ii) >= minTx)) {
                ii--;
            }
            lon_smooth_start = ii/2;
        }
        if (altitude_smooth_start == 0 && last_values.altitude != currentAttitude.altitude) {
            int ii = i;
            long double v = last_values.altitude;
            while (ii > 0 && (v == t_y.at(ii).altitude || t_x.at(ii) >= minTx)) {
                ii--;
            }
            altitude_smooth_start = ii/2;
        }
        i--;
        //last_values = t_y.at(i);
        //Ota tästä kommentti pois jos haluat ottaa tämän funkkarin käyttöön
    }

    plane_data smoothed = smooth(&t_y, pitch_smooth_start, bank_smooth_start, heading_smooth_start, lat_smooth_start, lon_smooth_start, altitude_smooth_start);

    m->at("HEr") = smoothed.bank;
    m->at("HGr") = smoothed.pitch;
    m->at("HCr") = smoothed.heading;
    m->at("HKr") = smoothed.lat;
    m->at("HMr") = smoothed.lon;
    m->at("HA") = smoothed.altitude;
}

bool validateMap(std::map<std::string, long double>* m) {
    if (m->count("HEr") && m->count("HGr") && m->count("HCr") && m->count("HKr") && m->count("HMr")) {
        return true;
    }
    return false;
}

void eventloop(const char* server, const char* port, LONG timeDelta) {
    // Request newest data from UDP server
    std::string data = "OK";
    std::vector<std::string> v;
    std::map<std::string, long double> m;


    Socket.SendTo(server, atoi(port), data.c_str(), data.size());
    Socket.RecvFrom(UDPBuffer, 1024);

    // Split data by ' '
    split(UDPBuffer, v, ' ');

    // parse vector into map (decode dataprotocol)
    parseVarsToMap(v, m);

    if (validateMap(&m)) {
        updatePrediction(&m, timeDelta);

        // Send data to simulator
        //printf("Altitude: %f, Lat: %f, Lon: %f, Pitch: %f, Bank: %f, Heading: %f, TDELTA: %ld \r", m["HA"], m["HKr"], m["HMr"], m["HEr"], m["HGr"], m["HCr"], timeDelta);
        sendDataToFSX(m["HA"], m["HKr"], m["HMr"], m["HEr"], m["HGr"], m["HCr"]);
    }
    else {
        printf("unvalid data\r\n");
    }
}



int main(int argc, const char* argv[])
{
    if (!initFSX() || argc < 3) {
        printf("Usage: %s [proxy IP] [proxy Port]\r\n", argv[0]);
        return -1;
    }
    SYSTEMTIME time, last_time;
    GetSystemTime(&time);
    last_time = time;
    unsigned int counter = 0;
    while (quit == 0) {
        GetSystemTime(&time);
        LONG delta = time.wSecond < last_time.wSecond ? (last_time.wSecond - 60) * 1000 + (1000 - last_time.wMilliseconds) + time.wSecond * 1000 + time.wMilliseconds : (time.wSecond - last_time.wSecond) * 1000 + time.wMilliseconds - last_time.wMilliseconds;
        
        eventloop(argv[1], argv[2],delta);
        SimConnect_CallDispatch(hSimConnect, MyDispatchProcSD, NULL);

        // if we're going "too fast" sleep a littlebit (Simconnect fix)
        if (delta < 5) {
            Sleep(5 - delta);
        }
        counter++;
        if (time.wSecond != last_time.wSecond) {
            printf("Updating %d times/s                                           \r", counter);
            counter = 0 ;
        }
        last_time = time;
    }
    SimConnect_Close(hSimConnect);

	return 0;
}