#pragma once
#include <TinyGPS++.h>
#include <Time.h>
#include "config.h"

/* GPS */
TinyGPSPlus gps;
// allocate the maximum buffer size, so we can reduce the Serial polling interval to a minimum
//SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN, false, 27*49);

void pollGPS() {
  while (Serial.available() > 0)
    gps.encode(Serial.read());  
}

bool updateLocation() {
  // abort if the GPS device does not push valid data  within one update cycle
  static const unsigned int timeout = 2 * GPS_INTERVAL;
  unsigned long start = millis();
  
  do {
    pollGPS();
    if (millis() - start > timeout) return false;
  } while (!gps.location.isUpdated());
  return true;
}

bool updateTime() {
  // abort if the GPS device does not push valid data  within one update cycle
  static const unsigned int timeout = 2 * GPS_INTERVAL;
  unsigned long start = millis();
  
  do {
    pollGPS();
    if (millis() - start > timeout) return false;
  } while ( !(gps.date.isUpdated() && gps.time.isUpdated()) );

  // in case we didnt timeout, resync the local clock
  setTime(gps.time.hour(), gps.time.minute(), gps.time.second(),
    gps.date.day(), gps.date.month(), gps.date.year());

  return true;
}


/* UTILS */
String getISODate() {
  char result[16] = { 0 };
  sprintf(result, "%04d-%02d-%02d-T%02d:%02d:%02dZ",
    year(), month(), day(), hour(), minute(), second());
  return (String)result;
}
