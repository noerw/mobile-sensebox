#pragma once
#include <ESP8266WiFi.h>
#include "config.h"

struct WifiState {
  bool homeAvailable;
  unsigned int numNetworks;
  unsigned int numUnencrypted;
};

class Wifi {
  public:
  void begin() {
    WiFi.mode(WIFI_STA);
  }
  
  WifiState scan(String homeSSID) {
    // TODO: filter duplicate SSIDs!
    WifiState state;
    state.homeAvailable = false;
    state.numNetworks = WiFi.scanNetworks(false, false);
    state.numUnencrypted = 0;
  
    for (unsigned int i = 0; i < state.numNetworks; i++) {
      if (WiFi.encryptionType(i) == ENC_TYPE_NONE)
        ++state.numUnencrypted;
  
      if (WiFi.SSID(i) == homeSSID)
        state.homeAvailable = true;
    }
  
    return state;
  }
  
  bool connect(const char* ssid, const char* pass) {
    static const unsigned int timeout = 10000; // abort after 10 secs
    unsigned long start = millis();
  
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
    //telnet.print("Connecting to WiFi.");
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
      delay(200);
      //telnet.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      //telnet.println("connected!");
      return true;
    }
    //telnet.println(" timeout");
    return false;
  }

  bool isConnected() {
    return (WiFi.status() == WL_CONNECTED);
  }
};

