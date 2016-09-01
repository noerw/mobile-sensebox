#include "config.h"
#include "gps.h"
#include "wifi.h"
#include "api.h"
#include "storage.h"
#include "TelnetPrint.h"

#define DEBUG_OUT Serial
//#define DEBUG_OUT telnet

//TelnetPrint telnet = TelnetPrint();
Storage storage = Storage();
Wifi wifi = Wifi();
OsemApi api = OsemApi();
Gps gps = Gps();

/* UTILS */
void printState(WifiState wifiState) {
  DEBUG_OUT.print("homeAvailable: ");
  DEBUG_OUT.println(wifiState.homeAvailable);
  DEBUG_OUT.print("numNetworks: ");
  DEBUG_OUT.println(wifiState.numNetworks);
  DEBUG_OUT.print("numUnencrypted: ");
  DEBUG_OUT.println(wifiState.numUnencrypted);

  DEBUG_OUT.print("lat: ");
  //DEBUG_OUT.print(gps.getLocation().lat(), 6);
  DEBUG_OUT.print(" lng: ");
  //DEBUG_OUT.println(gps.getLocation().lng(), 6);

  DEBUG_OUT.println(gps.getISODate());
  DEBUG_OUT.println("");
}

bool storeMeasurement(float lat, float lng, float value, const char* timeStamp, const char* sensorID) {
  Measurement m;
  m.lat = lat;
  m.lng = lng;
  m.value = value;
  strcpy(m.timeStamp, timeStamp);
  strcpy(m.sensorID, sensorID);

  return storage.add(m);
}


/* MAIN ENTRY POINTS */
void setup() {
  DEBUG_OUT.begin(115200);

  size_t bytesFree = storage.begin();
  //gps.begin();
  wifi.begin();
  
  wifi.connect(WIFI_SSID, WIFI_PASS);

  //delay(5000); // DEBUG oportunity to connect to network logger

  // wait until we got a first fix from GPS, and thus an initial time
  /*DEBUG_OUT.print("Getting GPS fix..");
  while (!gps.updateLocation()) { DEBUG_OUT.print("."); }
  DEBUG_OUT.print(" done! ");*/
  DEBUG_OUT.println(gps.getISODate());

  DEBUG_OUT.println("Setup done!\n");
  DEBUG_OUT.println("WiFi MAC            WiFi IP");
  DEBUG_OUT.print(WiFi.macAddress());
  DEBUG_OUT.print("   ");
  DEBUG_OUT.println(WiFi.localIP());
  
  DEBUG_OUT.print("SPIFF bytes free: ");
  DEBUG_OUT.println(bytesFree);
  
  digitalWrite(D9, HIGH); // DEBUG: integrated led? doesnt work
}

void loop() {
  //pollGPS();
  //DEBUG_OUT.pollClients();
  WifiState wifiState = wifi.scan(WIFI_SSID);
  char* dateString = gps.getISODate();

  // TODO: take other measurements (average them?)
/*
  if(!gps.updateLocation()) DEBUG_OUT.println("GPS timed out (location)");
  if(!gps.updateTime()) DEBUG_OUT.println("GPS timed out (time)");
*/

  // write measurements to file
  if (storeMeasurement(51.25345345, 7.89, wifiState.numNetworks, "2016-08-31T00:03:10Z", ID_SENSOR_WIFI)) {
    DEBUG_OUT.print("measurement stored! storage size: ");
  } else {
    DEBUG_OUT.print("measurement store failed! storage size: ");
  }
  DEBUG_OUT.println(storage.size());

  // TODO: connect to wifi, if available & not connected yet
  
  // then upload local data, remove from storage
  while (storage.size()) {
    String measure = storage.pop();
    api.postMeasurement(measure.substring(API_KEY_LENGTH + 1), measure.substring(0, API_KEY_LENGTH));
  }

  printState(wifiState);
  delay(5000);
}

