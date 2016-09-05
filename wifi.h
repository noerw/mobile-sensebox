#pragma once
#include <ESP8266WiFi.h>
#include "config.h"

struct WifiState {
  bool homeAvailable;
  unsigned int numAccessPoints;
  unsigned int numNetworks;
  unsigned int numUnencrypted;
};

class Wifi {
  public:
  void begin() {
    WiFi.mode(WIFI_STA);
  }
  
  WifiState scan(String homeSSID) {
    WifiState state;
    int n = WiFi.scanNetworks(false,false);
    state.numAccessPoints = n;
    state.numNetworks = n;
    state.numUnencrypted = 0;
    state.homeAvailable = false;
    int indices[n];
    String ssid;
    
    for (int i = 0; i < n; i++) indices[i] = i;

    for (int i = 0; i < n; i++) {
      // filter duplicates
      if(indices[i] == -1) {
        --state.numNetworks;
        continue;
      }
      ssid = WiFi.SSID(indices[i]);
      for (int j = i + 1; j < n; j++) {
        if (ssid == WiFi.SSID(indices[j])) indices[j] = -1;
      }
      
      if (indices[i] != -1) {
        if (WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE)
          ++state.numUnencrypted;
  
        if (WiFi.SSID(indices[i]) == homeSSID)
          state.homeAvailable = true;
      }
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

