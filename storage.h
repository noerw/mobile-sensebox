#pragma once

#include <FS.h>
#include <ESP8266TrueRandom.h>
#include "config.h"
#include "streampipe.h"

#define MEASUREMENT_JSON_SIZE (JSON_OBJECT_SIZE(4)) // 4 properties (sensorID is excluded)

struct Measurement {
  char timeStamp[20];
  float lat;
  float lng;
  float value;
  char sensorID[API_KEY_LENGTH];
};

class Storage {
  protected:
  void serializeMeasurement(Measurement& m, Print& f) {
    // prepend the sensor ID to the json
    f << m.sensorID << EOL;

    // convert floats to strings
    char val[10], lat[10], lng[10];
    dtostrf(m.value, 5, 6, val);
    dtostrf(m.lat, 5, 6, lat);
    dtostrf(m.lng, 5, 6, lng);
    
    f << "{\"value\":" << val
      << ",\"createdAt\":\"" << m.timeStamp
      << "\",\"lat\":" << lat
      << ",\"lng\":" << lng
      << "}" << EOL;
  }
  
  public:
  Storage() {}

  size_t begin() {
    SPIFFS.begin();
    FSInfo fs;
    SPIFFS.info(fs);
    return fs.totalBytes - fs.usedBytes;
  }
  
  bool add(Measurement& m, const char* directory = "/measurements/") {
    byte uuid[16];
    ESP8266TrueRandom.uuid(uuid);
    // we need to shorten the uuid, as long filenames are not supported it seems..?
    String fileName = directory + ESP8266TrueRandom.uuidToString(uuid).substring(26);
    
    if (File f = SPIFFS.open(fileName, "w") ) {
      serializeMeasurement(m, f);
      f.close();
      return true;
    }
    return false;
  }

  String pop(const char* directory = "/measurements/") {
    Dir dir = SPIFFS.openDir(directory);
    String measurement = "";
    if (!dir.next()) return measurement; // abort if storage is empty
    String fileName = dir.fileName();
    File f = dir.openFile("r");
    measurement = f.readString();
    f.close();
    SPIFFS.remove(fileName);
    return measurement;
  }

  uint16_t size(const char* directory = "/measurements/") {
    Dir dir = SPIFFS.openDir(directory);
    uint16_t i = 0;
    while(dir.next()) i++;
    return i;
  }
  
};

