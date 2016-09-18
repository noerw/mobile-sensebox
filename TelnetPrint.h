/**
 * sets up a telnet server, which can then be used for debug logging purposes
 * provides all Print methods.
 * begin()       must be called in the setup!
 * pollClients() must be called in the loop!
 * to disable the routines, just skip the begin() call in the setup
 */

# pragma once
#include <ESP8266WiFi.h>
#define MAX_TELNET_CLIENTS 3

class TelnetPrint : public Print {
  protected:
  WiFiServer server;
  WiFiClient clients[MAX_TELNET_CLIENTS];
  
  public:
  TelnetPrint(uint16_t port=23) : server(port) {}

  void begin(int baudProxy = 0) {
    server.begin();
    server.setNoDelay(false);
  }

  void pollClients() {
    //check clients for data
    for(uint16_t i = 0; i < MAX_TELNET_CLIENTS; i++){
      if (clients[i] && clients[i].connected()){
        while(clients[i].available()) clients[i].read();
      }
    }

    // try to establish pending connections
    if (server.hasClient()) {
      for(uint8_t i = 0; i < MAX_TELNET_CLIENTS; i++){
        //find free/disconnected spot
        if (!clients[i] || !clients[i].connected()){
          if(clients[i]) clients[i].stop();
          clients[i] = server.available();
          return;
        }
      }
      //no free/disconnected spot so reject
      WiFiClient serverClient = server.available();
      serverClient.stop();
    }      
  }

  // TODO: find more efficient method than sending byte by byte (!)
  virtual size_t write(uint8_t s) {
    for(uint8_t i = 0; i < MAX_TELNET_CLIENTS; i++){
      if (clients[i] && clients[i].connected()){
        clients[i].write(s);
      }
    }
    return 1;
  }

  // Is only called for Strings, not char arrays?
  /*virtual size_t write(const char *buffer, size_t size) {
    for(uint8_t i = 0; i < MAX_TELNET_CLIENTS; i++){
      if (clients[i] && clients[i].connected()){
        clients[i].write(buffer, size);
      }
    }
    return size;
  }*/
};

