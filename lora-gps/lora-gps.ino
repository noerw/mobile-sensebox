/**
   lora-gps senseBox with SDS011 particulate matter + HDC1008 temp & humi sensors
   for Arduino Mega with Dragino LoRa shield.
   SDS is on SDS_SERIAL, GPS on GPS_SERIAL
*/

#include "config.h"
#include "lora.h"
#include "sd.h"

#include <HDC100X.h>
#include <SDS011-select-serial.h>

#include <LoraMessage.h>

//#include <avr/wdt.h>

#include <TinyGPS++.h>
TinyGPSPlus gps;

//Load sensors
SDS011 my_sds(SDS_SERIAL);
HDC100X HDC(0x43);

//measurement variables
float temperature = 0, humidity = 0, p10 = 0, p25 = 0;
int error;

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    LoraMessage message;

    //-----GPS-----//
    bool newData = false;
    float lat, lng;
    for (unsigned long start = millis(); millis() - start < 5000;) // timeout
    {
      // TODO
      //while (gps.location.isUpdated() && gps.location.isValid())
      while (GPS_SERIAL.available())
      {
        char c = GPS_SERIAL.read();
        if (gps.encode(c)) // Did a new valid sentence come in?
          newData = true;
      }
    }
    if (newData) {
      lat = gps.location.lat();
      lng = gps.location.lng();
      Serial.print("latitude: ");
      Serial.println(lat, 6);
      Serial.print("longitude: ");
      Serial.println(lng, 6);
      message.addLatLng(lat, lng);
    } else {
      return;
    }

    delay(300);

    //-----Temperature-----//
    Serial.print("temperature: ");
    temperature = HDC.getTemp();
    Serial.println(temperature);
    message.addTemperature(temperature);
    delay(300);

    //-----Humidity-----//
    Serial.print("humidity: ");
    humidity = HDC.getHumi();
    Serial.println(humidity);
    message.addHumidity(humidity);
    delay(300);

    //-----fine dust-----/
    error = my_sds.read(&p25, &p10);
    if (!error) {
      message.addTemperature(p25);
      message.addTemperature(p10);
      Serial.println("P2.5: " + String(p25));
      Serial.println("P10:  " + String(p10));
    }

    Serial.println(F("Writing to SD card."));
    writeToSD(temperature, humidity, p25, p10, gps);

    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, message.getBytes(), message.getLength(), 0);
    Serial.println(F("LoRa Packet queued"));

  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void initSensors() {
  //Initialize sensors
  Serial.print("Initializing sensors...");
  Wire.begin();
  HDC.begin(HDC100X_TEMP_HUMI, HDC100X_14BIT, HDC100X_14BIT, DISABLE);
  Serial.println("done!");
  temperature = HDC.getTemp();

  // initialize GPS Serial Port
  GPS_SERIAL.begin(9600);
  while (!GPS_SERIAL.available()) {
    Serial.println("detecting GPS device...");
    delay(1000);
  }
  Serial.println("Wait for GPS...");
  while (!gps.location.isValid()) {
    gps.encode(GPS_SERIAL.read());
    delay(1);
  }
  Serial.println("Got GPS fix!");

  // init SD card
  Serial.print("Initializing SD card...");

  if (!SD.begin(SD_PIN)) {
    Serial.println("failed!");
    return;
  }
  Serial.println("done!");

  // initalize SDS Serial Port
  SDS_SERIAL.begin(9600);
}

void setup() {
  Serial.begin(9600); // debug serial
  Serial.println(F("Starting"));

#ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
#endif

  // sd card
  pinMode(4, INPUT);
  digitalWrite(4, HIGH);

  initSensors();

  Serial.println("initializing LoRa..");

  // LMIC LoRa init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // maximum tx power
  LMIC.txpow = 27;
  // slower datarate -> more stability. faster datarate -> lower airtime -> more packets
  // see https://docs.google.com/spreadsheets/d/1eL1nHxMidIcIdDE_l-DoY3kmE2e8b0YpOBW64WnYxj8
  LMIC.datarate = DR_SF10;

  Serial.println("Starting loop.");
  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
