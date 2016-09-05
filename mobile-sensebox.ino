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

bool storeMeasurement(float lat, float lng, float value, const char* timeStamp, const char* sensorID) {
  Measurement m;
  m.lat = lat;
  m.lng = lng;
  m.value = value;
  strcpy(m.timeStamp, timeStamp);
  strcpy(m.sensorID, sensorID);

  return storage.add(m);
}

bool uploadMeasurements(const uint16_t maxUploads = 5) {
  uint16_t uploadCount = 0;
  String measure;
  // only upload limited measures per cycle, to avoid long gaps in measurements
  while (storage.size() && uploadCount++ < maxUploads) {
    measure = storage.pop();
    DEBUG_OUT << "Uploading measurement for " << measure;
    if(!api.postMeasurement(measure.substring(API_KEY_LENGTH + 1), measure.substring(0, API_KEY_LENGTH))) {
      DEBUG_OUT << "upload failed!" << EOL;
      return false;
    }
    DEBUG_OUT << "success!" << EOL;
  }
  return true;       
}

// delay for a given duration (ms), rollover-safe implementation
// offset may be a duration which has been "used up" before, so the delay is adaptive,
// meaning that the interval of a adaptiveDelay() call is constant
void adaptiveDelay(unsigned long ms, unsigned long offset = 0) {
  unsigned long start = millis();
  for (;;) {
    // for some reason, operations have to be performed in this loop for
    // proper operation, so we just do the polling to be done anyway
    gps.pollGPS();
    telnet.pollClients();
  
    unsigned long now = millis();
    unsigned long elapsed = now - start + offset;
    //DEBUG_OUT << elapsed << EOL;
    if (elapsed >= ms) return;
  }
}

/* MAIN ENTRY POINTS */
void setup() {
  size_t bytesFree = storage.begin();
  gps.begin();
  wifi.begin();
  
  // DEBUG: just for connection to telnet printer
  wifi.connect(WIFI_SSID, WIFI_PASS);
  DEBUG_OUT.begin();
  delay(5000);
  telnet.pollClients();

  // wait until we got a first fix from GPS, and thus an initial time
  DEBUG_OUT.print("Getting GPS fix..");
  while (!gps.updateLocation()) { DEBUG_OUT.print("."); }
  DEBUG_OUT.println(" done! ");

  DEBUG_OUT.println("WiFi MAC            WiFi IP");
  DEBUG_OUT.print(WiFi.macAddress());
  DEBUG_OUT.print("   ");
  DEBUG_OUT.println(WiFi.localIP());
  
  DEBUG_OUT.print("SPIFF bytes free: ");
  DEBUG_OUT.println(bytesFree);
  
  DEBUG_OUT.println("Setup done!\n");
}

unsigned long cycleStart;
TinyGPSLocation loc;
char dateString[20];

void loop() {
  cycleStart = millis();
  
  //gps.pollGPS();
  //telnet.pollClients();

  // measure WiFi
  WifiState wifiState = wifi.scan(WIFI_SSID);

  // measure GPS
  if(!gps.updateLocation()) DEBUG_OUT.println("GPS timed out (location)");
  if(!gps.updateTime()) DEBUG_OUT.println("GPS timed out (time)");
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

  // TODO. write location update to file  
  
  // write measurements to file
  if (storeMeasurement(loc.lat(), loc.lng(), wifiState.numAccessPoints, dateString, ID_SENSOR_WIFI_APS)) {
    DEBUG_OUT.print("measurement (wifi aps) stored! storage size: ");
  } else {
    DEBUG_OUT.print("measurement (wifi aps) store failed! storage size: ");
  }
  DEBUG_OUT.println(storage.size());
  if (storeMeasurement(loc.lat(), loc.lng(), wifiState.numNetworks, dateString, ID_SENSOR_WIFI_NET)) {
    DEBUG_OUT.print("measurement (wifi nets) stored! storage size: ");
  } else {
    DEBUG_OUT.print("measurement (wifi nets) store failed! storage size: ");
  }
  DEBUG_OUT.println(storage.size());
  if (storeMeasurement(loc.lat(), loc.lng(), wifiState.numUnencrypted, dateString, ID_SENSOR_WIFI_OPEN)) {
    DEBUG_OUT.print("measurement (wifi open) stored! storage size: ");
  } else {
    DEBUG_OUT.print("measurement (wifi open) store failed! storage size: ");
  }
  DEBUG_OUT.println(storage.size());
  
  // connect to wifi, if available & not connected yet
  // once we are connected, upload (max) 4 stored measurements & free the storage
  if (wifi.isConnected() || (wifiState.homeAvailable && wifi.connect(WIFI_SSID, WIFI_PASS)) ) {
    uploadMeasurements(4);  
  }
  
  DEBUG_OUT << EOL;

  // the measurement & upload cycle takes ~4 seconds, so lets we wait measure
  // every 10secs in total
  // run the loop every 10secs. using an adaptive delay
  // IDEA: dont use time but distance interval? -> TinyGPSPlus::distanceBetween()
  adaptiveDelay(MEASUREMENT_INTERVAL, millis() - cycleStart);
}

