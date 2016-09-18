#pragma once

/* GENERAL */
// interval of the measurements in millisec / meters
#define MEASUREMENT_INTERVAL 12000 // 0 for "as fast as possible"
#define MEASUREMENT_DISTANCE_ENABLED true
#define MEASUREMENT_DISTANCE 25

// pull this pin down to disable measurements (eg for quickly bulk uploading stored measurements)
#define PIN_MEASURE_MODE D14
// pull this pin down to disable uploads (eg for higher measurement rates when no realtime data is required)
#define PIN_UPLOAD_MODE D15

// should be higher than the number of sensors to avoid storage overflowing. tradeoff with the measurement interval
#define MAX_UPLOADS_PER_CYCLE 4

/* WiFi (ESP8266) */
#define WIFI_SSID "GIATSCHOOL-NET"
#define WIFI_PASS "XXXXXX"

/* GPS reciever (uBlox NEO-7M) connected to hardware serial (SoftwareSerial does not work well!!) */
#define GPS_BAUD 9600
#define GPS_INTERVAL 1000 // update interval of the gps device in ms

/* API (openSenseMap) */
#define API_ENDPOINT "api.osem.vo1d.space"
// SHA1 of the API SSL cert
#define API_FINGERPRINT "A2 38 74 C7 B0 71 07 D4 2A 1C A5 6D 0D 05 3E 0A 90 68 A5 CB"
#define API_KEY "XXXXXXXXXXX"
#define ID_BOX "57c7f3291421551100bf13c8"
#define ID_SENSOR_WIFI_APS "57c7f3291421551100bf13ca"
#define ID_SENSOR_WIFI_NET "57cdd4ce1421551100bf17c5"
#define ID_SENSOR_WIFI_OPEN "57cdd4ce1421551100bf17c6"
