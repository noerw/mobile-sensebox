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
  /*
   * connects to the provided network. 
   * if the connection is lost, it will attempt to reconnect,
   * when the SSID is found again
   * 
   * also sets up an open AP "mobile-sensebox", on which you
   * you can get debug output via telnet on 192.168.1.1:23
   */
  void begin(const char* ssid, const char* pass) {
    const IPAddress AP_IP(192, 168, 1, 1);
    const IPAddress AP_gateway(192, 168, 1, 1);
    const IPAddress AP_subnet(255, 255, 255, 0);
    
    WiFi.mode(WIFI_AP_STA);
    WiFi.persistent(false);       //
    WiFi.setAutoConnect(false);   // <-- weird?!
    WiFi.setAutoReconnect(false); //
    WiFi.begin(ssid, pass);
    WiFi.softAPConfig (AP_IP, AP_gateway, AP_subnet);
    WiFi.softAP("mobile-sensebox");
  }
  
  WifiState scan(String homeSSID) {
    WifiState state;
    int n = WiFi.scanNetworks(false, true); // (execute async, list hidden networks)
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
    
    WiFi.scanDelete();
    return state;
  }

  bool reconnect() {
    return WiFi.reconnect();
  }

  bool isConnected() {
    return (WiFi.status() == WL_CONNECTED);
  }
};

