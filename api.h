#pragma once
#include <WiFiClientSecure.h>
#include "config.h"

WiFiClientSecure api;

bool postMeasurement(String m, String sensorID) {
  //telnet.print("Connecting to API.. ");
  if (!api.connect(API_ENDPOINT, 443)) {
    //telnet.println("connection failed");
    return false;
  }
  if (api.verify(API_FINGERPRINT, API_ENDPOINT)) {
    //telnet.println("certificate matches");
  } else {
    //telnet.println("certificate doesn't match");
    return false;
  }

  String url = "/boxes/" + String(ID_BOX) + "/" + sensorID;
  api.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + API_ENDPOINT + "\r\n" +
               "User-Agent: mobile-sensebox-esp8266\r\n" +
               "Connection: close\r\n\r\n");

  return true;
}
