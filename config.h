/* WiFi (ESP8266) */
#pragma once

static const char* WIFI_SSID = "Elmo";
static const char* WIFI_PASS = "FreiBier123";

/* GPS reciever (uBloc NEO-7M) */
static const int   GPS_RX_PIN = 4;
static const int   GPS_TX_PIN = 3;
static const int   GPS_BAUD = 9600;
static const int   GPS_INTERVAL = 1000; // update interval of the gps device

/* API (openSenseMap) */
static const char* API_ENDPOINT = "api.opensensemap.org";
// SHA1 of the API SSL cert
static const char* API_FINGERPRINT = "0F B0 0C E0 FD 18 C2 0B 07 1C 21 AB A0 FF EF CC 09 62 57 A9";
static const char* API_KEY = "...";
static const char* ID_BOX = "57308b2c566b8d3c11114a9f";
static const char* ID_SENSOR_WIFI = "...";
