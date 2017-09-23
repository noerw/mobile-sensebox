#pragma once

#include <FS.h>
#include <ESP8266TrueRandom.h>
#include "config.h"
#include "streampipe.h"

struct Measurement {
  char timeStamp[20];
  double lat;
  double lng;
  float value;
  char sensorID[24];
};

class Storage {
  protected:
  void serializeMeasurement(Measurement& m, Print& f) {
    // prepend the sensor ID to the json
    f << m.sensorID << '\n';

    // convert floats to strings
    char val[8], lat[16], lng[16];
    dtostrf(m.value, 6, 2, val);
    dtostrf(m.lat, 12, 8, lat);
    dtostrf(m.lng, 12, 8, lng);

    f << "{\"value\":" << val
      << ",\"createdAt\":\"" << m.timeStamp
      << "\",\"location\":[" << lng << "," << lat
      << "]}" << EOL;
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

  String get(String& fileName, boolean remove = false, const char* directory = "/measurements/") {
    Dir dir = SPIFFS.openDir(directory);
    String measurement = "";
    if (!dir.next()) return measurement; // abort if storage is empty
    fileName = dir.fileName();
    File f = dir.openFile("r");
    measurement = f.readString();
    f.close();
    if (remove) SPIFFS.remove(fileName);
    return measurement;
  }

  bool remove(String& fileName, const char* directory = "/measurements/") {
    return SPIFFS.remove(fileName);
  }

  uint16_t size(const char* directory = "/measurements/") {
    Dir dir = SPIFFS.openDir(directory);
    uint16_t i = 0;
    while(dir.next()) i++;
    return i;
  }

};

