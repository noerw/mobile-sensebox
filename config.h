#pragma once

/* WiFi (ESP8266) */


#define WIFI_SSID "Penaten"
#define WIFI_PASS "XXXXXX"

/* GPS reciever (uBloc NEO-7M) */
#define GPS_RX_PIN 4
#define GPS_TX_PIN 3
#define GPS_BAUD 9600
#define GPS_INTERVAL 1000 // update interval of the gps device

/* API (openSenseMap) */
#define API_ENDPOINT "api.osem.vo1d.space"
// SHA1 of the API SSL cert
#define API_FINGERPRINT "A2 38 74 C7 B0 71 07 D4 2A 1C A5 6D 0D 05 3E 0A 90 68 A5 CB"
#define API_KEY_LENGTH 24
#define API_KEY "XXXXXXXXXXX"
#define ID_BOX "57c7f3291421551100bf13c8"
#define ID_SENSOR_WIFI "57c7f3291421551100bf13ca"
