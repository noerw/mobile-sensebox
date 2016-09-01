#pragma once
#include <WiFiClientSecure.h>
#include "config.h"
#include "streampipe.h"

class OsemApi {
  protected:
  WiFiClientSecure client;

  public:
  bool postMeasurement(String measurement, String sensorID) {
    //telnet.print("Connecting to API.. ");
    if (!client.connect(API_ENDPOINT, 443)) {
      //Serial.println("connection failed");
      return false;
    }
    
    if (!client.verify(API_FINGERPRINT, API_ENDPOINT)) {
      //Serial.println("certificate doesn't match");
      return false;
    }
    
    client << String("POST ") << "/boxes/" << ID_BOX << "/" << sensorID << " HTTP/1.1" << EOL;
    client << "Host: " << API_ENDPOINT << EOL;
    client << "X-APIKey: " << API_KEY << EOL;
    client << "Content-Type: application/json" << EOL;
    client << "Connection: close" << EOL;
    client << "Content-Length: " << measurement.length() << EOL << EOL;
    client << measurement;

    // read response
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }
    Serial << "API-Server response: " << client.readString() << EOL;

    return true;
  }
};

