#include <TinyGPS++.h>
#include "lib/BME280/BME280I2C.h"
#include "config.h"
#include "gps.h"
#include "wifi.h"
#include "api.h"
#include "storage.h"
#include "TelnetPrint.h"
#include "streampipe.h"

#define DEBUG_OUT telnet // debug output is available via telnet!

Storage storage = Storage();
Wifi wifi = Wifi();
TelnetPrint telnet = TelnetPrint();
OsemApi api = OsemApi();
Gps gps = Gps();

BME280I2C bme(1, 1, 1, 3, 5, 0, false, 0x77);

unsigned long cycleStart;
WifiState wifiState; // global, as both measure and upload need the state
TinyGPSLocation location;

bool storeMeasurement(float lat, float lng, float value, const char* timeStamp, const char* sensorID) {
  Measurement m;
  m.lat = lat;
  m.lng = lng;
  m.value = value;
  strcpy(m.timeStamp, timeStamp);
  strcpy(m.sensorID, sensorID);
  return storage.add(m);
}

void measure(WifiState& wifiState, TinyGPSLocation& loc) {
  char dateString[22];
  float temperature, humid, pressure;

  // measure WiFi
  wifiState = wifi.scan(WIFI_SSID);

  // update gps position. if we can't get a fix, skip the measurements
  if(!gps.updateLocation() || !gps.updateTime()) {
    DEBUG_OUT << "GPS timed out, skipping measurements" << EOL;
    digitalWrite(BUILTIN_LED, LOW); // turn status LED on
    return;
  }

  digitalWrite(BUILTIN_LED, HIGH);
  loc = gps.getLocation();
  gps.getISODate(dateString);

  bme.read(pressure, temperature, humid, 1, true);

  // print state
  DEBUG_OUT << "homeAvailable:  " << wifiState.homeAvailable << EOL;
  DEBUG_OUT << "numAPs:         " << wifiState.numAccessPoints << EOL;
  DEBUG_OUT << "numNetworks:    " << wifiState.numNetworks << EOL;
  DEBUG_OUT << "numUnencrypted: " << wifiState.numUnencrypted << EOL;
  DEBUG_OUT << "temperature: " << temperature << EOL;
  DEBUG_OUT << "pressure: " << pressure << EOL;

  DEBUG_OUT.print("lat: ");
  DEBUG_OUT.print(loc.lat(), 8);
  DEBUG_OUT.print(" lng: ");
  DEBUG_OUT.println(loc.lng(), 8);
  DEBUG_OUT << dateString << EOL;

  // IDEA: write location update to separate file?

  // write measurements to file
  if (!storeMeasurement(loc.lat(), loc.lng(), wifiState.numAccessPoints, dateString, ID_SENSOR_WIFI_APS))
    DEBUG_OUT << "measurement (wifi aps) store failed!" << EOL;

  if (!storeMeasurement(loc.lat(), loc.lng(), wifiState.numNetworks, dateString, ID_SENSOR_WIFI_NET))
    DEBUG_OUT << "measurement (wifi nets) store failed!" << EOL;

  if (!storeMeasurement(loc.lat(), loc.lng(), wifiState.numUnencrypted, dateString, ID_SENSOR_WIFI_OPEN))
    DEBUG_OUT << "measurement (wifi open) store failed!" << EOL;

  if (!storeMeasurement(loc.lat(), loc.lng(), temperature, dateString, ID_SENSOR_TEMP))
    DEBUG_OUT << "measurement (temperature) store failed!" << EOL;

  if (!storeMeasurement(loc.lat(), loc.lng(), pressure, dateString, ID_SENSOR_PRESSURE))
    DEBUG_OUT << "measurement (pressure) store failed!" << EOL;

  DEBUG_OUT << EOL;
}

void upload(WifiState& wifiState) {
  bool connected = wifi.isConnected();
  size_t numMeasures = storage.size();
  DEBUG_OUT << numMeasures << " measurements stored." << EOL;
  if (!numMeasures) return;

  // in case we are not measuring, scan manually to detect the home network
  // somehow the automatic reconnect does not work.. TODO
  if (!connected && digitalRead(PIN_MEASURE_MODE) == LOW)
    wifiState = wifi.scan(WIFI_SSID);

  // connect to wifi, if available & not connected yet
  // once we are connected, upload stored measurements & free the storage
  // only upload limited measures per cycle, to avoid long gaps in measurements
  if (connected || (wifiState.homeAvailable && wifi.reconnect()) ) {
    uint16_t uploadCount = 0;
    String measure;
    int separatorPos;
    String fileName;
    while (storage.size() && uploadCount++ < MAX_UPLOADS_PER_CYCLE) {
      measure = storage.get(fileName);
      separatorPos = measure.indexOf('\n');
      DEBUG_OUT << "Uploading measurement for " << measure;
      if (api.postMeasurement(measure.substring(separatorPos + 1), measure.substring(0, separatorPos))) {
        // remove the measurement from the local storage on success
        DEBUG_OUT << "success!" << EOL;
        storage.remove(fileName);
      } else {
        DEBUG_OUT << "upload failed!" << EOL;
      }
    }
  } else {
    DEBUG_OUT << "wifi connection to " << WIFI_SSID <<  " failed" << EOL;
  }
  DEBUG_OUT << EOL;
}

/**
 * delay for a given duration (ms), rollover-safe implementation
 * offset may be a duration which has been "used up" before, so the delay is adaptive,
 * meaning that the interval of a adaptiveDelay() call is constant
 * returns earlier, if we moved more than MEASUREMENT_DISTANCE meters from our last fix
 *
 * also polls the GPS serial & telnet connections!
 */
void adaptiveDelay(unsigned long ms, TinyGPSLocation& lastLoc, unsigned long offset = 0, bool checkDistance = false) {
  unsigned long start = millis();
  for (;;) {
    // for some reason, operations have to be performed in this loop for
    // proper operation, so we just do the polling to be done anyway
    gps.pollGPS();
    telnet.pollClients();

    // update our location. if we moved MEASUREMENT_DISTANCE meters or more, return
    TinyGPSLocation newLoc = gps.getLocation();
    double distanceToPrevLoc = TinyGPSPlus::distanceBetween(lastLoc.lat(), lastLoc.lng(), newLoc.lat(), newLoc.lng());
    if (checkDistance && distanceToPrevLoc >= MEASUREMENT_DISTANCE) {
      DEBUG_OUT << "moved  " << distanceToPrevLoc  << "m, delay stopped." << EOL;
      return;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - start + offset;
    if (elapsed >= ms) return;
  }
}


/* MAIN ENTRY POINTS */
void setup() {
  pinMode(BUILTIN_LED, OUTPUT); // status indicator LED: on = no GPS fix
  digitalWrite(BUILTIN_LED, LOW);
  pinMode(PIN_MEASURE_MODE, INPUT_PULLUP); // switch for measurements (pull it down to disable)
  pinMode(PIN_UPLOAD_MODE, INPUT_PULLUP);  // switch for API uploads  (pull it down to disable)

  size_t bytesFree = storage.begin();
  gps.begin();
  wifi.begin(WIFI_SSID, WIFI_PASS);
  DEBUG_OUT.begin(115200);

  while(!bme.begin(D14, D15)){
    adaptiveDelay(1000, location);
    DEBUG_OUT.println("Could not find BME280I2C sensor!");
  }

  // wait until we got a first fix from GPS, and thus an initial time
  // exception: measure mode is disabled (for quick upload only)
  if (digitalRead(PIN_MEASURE_MODE) == HIGH) {
    DEBUG_OUT << "Getting GPS fix..";
    while (!gps.updateLocation()) {
      adaptiveDelay(0, location); // poll for telnet connections
      DEBUG_OUT << ".";
    }
    location = gps.getLocation();
    DEBUG_OUT << " done!" << EOL;
    digitalWrite(BUILTIN_LED, HIGH);
  }

  // DEBUG
  //while (!wifi.isConnected()) adaptiveDelay(500, location);
  //String temp;  while (storage.size()) storage.get(temp, true);

  DEBUG_OUT << "Setup done!" << EOL;
  DEBUG_OUT << "WiFi MAC            WiFi IP" << EOL;
  DEBUG_OUT << WiFi.macAddress() << "   " << WiFi.localIP() << EOL;
  DEBUG_OUT << "SPIFF bytes free: " << bytesFree << EOL << EOL;
}

void loop() {
  cycleStart = millis();

  adaptiveDelay(0, location); // do some polling inbetween

  if (digitalRead(PIN_MEASURE_MODE) == HIGH)
    measure(wifiState, location);

  adaptiveDelay(0, location); // do some polling inbetween

  if (digitalRead(PIN_UPLOAD_MODE) == HIGH)
    upload(wifiState);

  // run the measurements in a fixed interval, using an adaptive delay
  // the interval is defined by a duration and/or distance from our last fix
  if (digitalRead(PIN_MEASURE_MODE) == HIGH)
    return adaptiveDelay(MEASUREMENT_INTERVAL, location, millis() - cycleStart, MEASUREMENT_DISTANCE_ENABLED);

  // run as fast as possible when not measuring.
  // smartDelay has to be called anyway, as some polling functions are run within
  adaptiveDelay(0, location);
}

