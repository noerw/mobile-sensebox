#pragma once
#include <TinyGPS++.h>
#include <Time.h>
#include "config.h"

// TODO: refactor updateXXX members to be protected and called by getXXX ?

class Gps {
  protected:
  TinyGPSPlus gps;
  
  public:
  void begin() {
    Serial.begin(GPS_BAUD);
  }
  
  /** 
   * parse all available bytes from the Serial input
   * should be called frequently to avoid buffer overflows
   */
  void pollGPS() {
    while (Serial.available() > 0)
      gps.encode(Serial.read());  
  }

  /**
   * poll until we have a valid location.
   * retries at most once, returns success state
   */
  bool updateLocation() {
    // abort if the GPS device does not push valid data  within one update cycle
    static const unsigned int timeout = 2 * GPS_INTERVAL;
    unsigned long start = millis();
    
    do {
      pollGPS();
      if (millis() - start > timeout) return false;
    } while (!gps.location.isUpdated()); // TODO: check if is valid?
    return true;
  }
  
  /**
   * poll until we have a valid date & time.
   * retries at most once, returns success state
   */
  bool updateTime() {
    // abort if the GPS device does not push valid data  within one update cycle
    static const unsigned int timeout = 2 * GPS_INTERVAL;
    unsigned long start = millis();

    // TODO: check if is valid?
    do {
      pollGPS();
      if (millis() - start > timeout) return false;
    } while ( !(gps.date.isUpdated() && gps.time.isUpdated()) );
  
    // in case we didnt timeout, resync the local clock (Time.h)
    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(),
      gps.date.day(), gps.date.month(), gps.date.year());
  
    return true;
  }

  TinyGPSLocation& getLocation() {
    return gps.location;
  }
  
  /**
   * return an iso8601 formatted datestring with the current time
   */
  static char* getISODate() {
    // TODO: check why we need to allocate that much storage for a 20 character string for Serial printing??
    char result[100] = { 0 };
    sprintf(result, "%04d-%02d-%02d-T%02d:%02d:%02dZ",
      year(), month(), day(), hour(), minute(), second());
    return result;
  }
};

