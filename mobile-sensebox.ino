#include <TinyGPS++.h>
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
  char dateString[20];
  
  // measure WiFi
  wifiState = wifi.scan(WIFI_SSID);

  // update gps position if we can't get a fix, skip the measurements
  if(!gps.updateLocation() || !gps.updateTime()) {
    DEBUG_OUT << "GPS timed out, skipping measurements" << EOL;
    digitalWrite(BUILTIN_LED, LOW); // turn status LED on
    return;
  }

  digitalWrite(BUILTIN_LED, HIGH);
  loc = gps.getLocation();
  gps.getISODate(dateString);

  // print state
  DEBUG_OUT << "homeAvailable:  " << wifiState.homeAvailable << EOL;
  DEBUG_OUT << "numAPs:         " << wifiState.numAccessPoints << EOL;
  DEBUG_OUT << "numNetworks:    " << wifiState.numNetworks << EOL;
  DEBUG_OUT << "numUnencrypted: " << wifiState.numUnencrypted << EOL;
  DEBUG_OUT.print("lat: ");
  DEBUG_OUT.print(loc.lat(), 6);
  DEBUG_OUT.print(" lng: ");
  DEBUG_OUT.println(loc.lng(), 6);
  DEBUG_OUT << dateString << EOL;

  // IDEA: write location update to separate file?
  
  // write measurements to file
  if (!storeMeasurement(loc.lat(), loc.lng(), wifiState.numAccessPoints, dateString, ID_SENSOR_WIFI_APS))
    DEBUG_OUT << "measurement (wifi aps) store failed!" << EOL;

  if (!storeMeasurement(loc.lat(), loc.lng(), wifiState.numNetworks, dateString, ID_SENSOR_WIFI_NET))
    DEBUG_OUT << "measurement (wifi nets) store failed!" << EOL;

  if (!storeMeasurement(loc.lat(), loc.lng(), wifiState.numUnencrypted, dateString, ID_SENSOR_WIFI_OPEN))
    DEBUG_OUT << "measurement (wifi open) store failed!" << EOL;
    
  DEBUG_OUT << EOL;
}

void upload(WifiState& wifiState) {
  size_t numMeasures = storage.size();
  DEBUG_OUT << numMeasures << " measurements stored." << EOL;
  if (!numMeasures) return;
  
  // in case we are not measuring, scan manually to detect the home network
  if (digitalRead(PIN_MEASURE_MODE) == LOW)
    wifiState = wifi.scan(WIFI_SSID);
    
  // connect to wifi, if available & not connected yet
  // once we are connected, upload (max) 5 stored measurements & free the storage
  if (wifi.isConnected() || (wifiState.homeAvailable && wifi.connect(WIFI_SSID, WIFI_PASS)) ) {
    uint16_t uploadCount = 0;
    String measure;
    // only upload limited measures per cycle, to avoid long gaps in measurements
    while (storage.size() && uploadCount++ < MAX_UPLOADS_PER_CYCLE) {
      measure = storage.pop();
      DEBUG_OUT << "Uploading measurement for " << measure;
      if (api.postMeasurement(measure.substring(API_KEY_LENGTH + 1), measure.substring(0, API_KEY_LENGTH)))
        DEBUG_OUT << "success!" << EOL;
      else
        DEBUG_OUT << "upload failed!" << EOL;
    }
  } else {
    DEBUG_OUT << "wifi connection to " << WIFI_SSID <<  " failed" << EOL;
  }
  DEBUG_OUT << EOL;
}

// delay for a given duration (ms), rollover-safe implementation
// offset may be a duration which has been "used up" before, so the delay is adaptive,
// meaning that the interval of a adaptiveDelay() call is constant
// returns earlier, if we moved more than MEASUREMENT_DISTANCE meters from our last fix
void adaptiveDelay(unsigned long ms, TinyGPSLocation& lastLoc, unsigned long offset = 0) {
  unsigned long start = millis();
  for (;;) {
    // for some reason, operations have to be performed in this loop for
    // proper operation, so we just do the polling to be done anyway
    gps.pollGPS();
    telnet.pollClients();

    // update our location. if we moved MEASUREMENT_DISTANCE meters or more, return
    TinyGPSLocation newLoc = gps.getLocation();
    double distanceToPrevLoc = TinyGPSPlus::distanceBetween(lastLoc.lat(), lastLoc.lng(), newLoc.lat(), newLoc.lng());
    if (MEASUREMENT_DISTANCE_ENABLED && distanceToPrevLoc >= MEASUREMENT_DISTANCE) {
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
  pinMode(BUILTIN_LED, OUTPUT);   // status indicator LED: on = no GPS fix
  digitalWrite(BUILTIN_LED, LOW);
  pinMode(PIN_MEASURE_MODE, INPUT_PULLUP); // switch for measurements (pull it down to disable)
  pinMode(PIN_UPLOAD_MODE, INPUT_PULLUP);  // switch for API uploads  (pull it down to disable)
  
  size_t bytesFree = storage.begin();
  gps.begin();
  wifi.begin();
  
  // DEBUG: just for connection to telnet printer
  wifi.connect(WIFI_SSID, WIFI_PASS);
  DEBUG_OUT.begin();
  delay(3000);
  telnet.pollClients();

  // wait until we got a first fix from GPS, and thus an initial time
  DEBUG_OUT << "Getting GPS fix..";
  while (!gps.updateLocation()) { DEBUG_OUT << "."; }
  location = gps.getLocation();
  DEBUG_OUT << " done!" << EOL;
  digitalWrite(BUILTIN_LED, HIGH);

  DEBUG_OUT << "Setup done!" << EOL;
  DEBUG_OUT << "WiFi MAC            WiFi IP" << EOL;
  DEBUG_OUT << WiFi.macAddress() << "   " << WiFi.localIP() << EOL;
  DEBUG_OUT << "SPIFF bytes free: " << bytesFree << EOL << EOL;
}

void loop() {
  cycleStart = millis();
  
  if (digitalRead(PIN_MEASURE_MODE) == HIGH)
    measure(wifiState, location);
  
  if (digitalRead(PIN_UPLOAD_MODE) == HIGH)
    upload(wifiState);
  
  if (digitalRead(PIN_MEASURE_MODE) == HIGH) {
    // run the measurements in a fixed interval, using an adaptive delay
    // the interval is defined by a duration and/or distance from our last fix
    return adaptiveDelay(MEASUREMENT_INTERVAL, location, millis() - cycleStart);
  }
  // run as fast as possible when not measuring.
  // smartDelay has to be called anyway, as some polling functions are run within
  adaptiveDelay(0, location);
}

