#pragma once
#include <WiFiClientSecure.h>
#include "config.h"
#include "streampipe.h"

class OsemApi {
  protected:
  WiFiClientSecure client;

  public:
  bool postMeasurement(String measurement, String sensorID) {
    if (!client.connect(API_ENDPOINT, 443)) {
      return false;
    }
    
    if (!client.verify(API_FINGERPRINT, API_ENDPOINT)) {
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
    if (!client.connected()) return false;
    String line = client.readStringUntil('\r');
    if (line != "HTTP/1.1 201 Created") return false;
   
    return true;
  }
};

