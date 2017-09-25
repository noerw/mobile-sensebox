/*
  based on https://github.com/noerw/sodaqone-gpslogger/blob/master/sodaq-gpslogger.ino
  adapted for Arduino Mega + GPS + SDS011 + HDC1008 + SD Card reader
  
  Copyright (c) 2017 Norwin Roosen, Felix Erdmann

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 */

#include <SD.h>
#include "TinyGPS++.h"
#include <HDC100X.h>
#include <SDS011-select-serial.h>

// change to Serial to disable log output.
#define DEBUG_OUT Serial2
#define SD_CHIPSELECT 4

#define MEASURE_INTERVAL 15000

//Load sensors
SDS011 sds(Serial);
HDC100X HDC(0x43);
TinyGPSPlus gps;

File logfile;
String logfile_path;
uint32_t cyclestart = 0;
double lastLat = 0;
double lastLng = 0;

//measurement variables
float temperature = 0, humidity = 0, pm10 = 0, pm25 = 0;
int error;

/**
 * rollover safe implementation, with an optional offset
 */
void adaptive_delay(uint32_t duration, uint32_t offset = 0) {
  unsigned long start = millis();
  for (;;) {
    unsigned long now = millis();
    unsigned long elapsed = now - start + offset;
    if (elapsed >= duration) return;
  }
}

/**
 * copied from SD example sketch
 * created  28 Mar 2011
 * by Limor Fried
 * modified 9 Apr 2012
 * by Tom Igoe
 */
bool check_sdcard (uint8_t chipSelect) {
  // set up variables using the SD utility library functions:
  Sd2Card card;
  SdVolume volume;
  SdFile root;

  DEBUG_OUT.print("\nInitializing SD card...");

  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    DEBUG_OUT.println("initialization failed. Things to check:");
    DEBUG_OUT.println("* is a card inserted?");
    DEBUG_OUT.println("* is your wiring correct?");
    DEBUG_OUT.println("* did you change the chipSelect pin to match your shield or module?");
    return false;
  } else {
    DEBUG_OUT.println("Wiring is correct and a card is present.");
  }

  // print the type of card
  DEBUG_OUT.print("\tCard type: ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      DEBUG_OUT.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      DEBUG_OUT.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      DEBUG_OUT.println("SDHC");
      break;
    default:
      DEBUG_OUT.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    DEBUG_OUT.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return false;
  }

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  DEBUG_OUT.print("\tVolume type is FAT");
  DEBUG_OUT.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();  // clusters are collections of blocks
  volumesize *= volume.clusterCount();     // we'll have a lot of clusters
  volumesize *= 512;                       // SD card blocks are always 512 bytes
  DEBUG_OUT.print("\tVolume size (bytes): ");
  DEBUG_OUT.print(volumesize);
  DEBUG_OUT.print(" bytes (");
  volumesize /= 1024;
  volumesize /= 1024;
  DEBUG_OUT.print(volumesize);
  DEBUG_OUT.println(" MB)");

  // TODO: show remaining bytes?

  //SerialUSB.println("\tFiles found on the card (name, date and size in bytes): ");
  //root.openRoot(volume);
  // list all files in the card with date and size
  //root.ls(LS_R | LS_DATE | LS_SIZE);

  return true;
}

void pollGPS() {
  while (Serial3.available() > 0) {
    gps.encode(Serial3.read());
  }
}

/**
 * poll until we have a valid location.
 * retries at most once, returns success state
 */
bool updateLocation() {
  // abort if the GPS device does not push valid data within one update cycle
  static const unsigned int timeout = 2000;
  unsigned long start = millis();

  do {
    pollGPS();
    if (millis() - start > timeout) return false;
  } while (!gps.location.isUpdated() && !gps.location.isValid() && gps.location.age() >= 1000);
  
  return true;
}

String make_logfile_path() {
  // filenames longer than 12 chars cannot be written..??
  return "datalog.csv";
  //return sodaq_gps.getDateTimeString().substring(0, 8) + ".csv";
}

void write_log_to_stream(Print &stream) {
  logfile.print(gps.date.year());
  logfile.print(F("-"));
  if (gps.date.month() < 10) logfile.print(F("0"));
  logfile.print(gps.date.month());
  logfile.print(F("-"));
  if (gps.date.day() < 10) logfile.print(F("0"));
  logfile.print(gps.date.day());
  logfile.print(F("T"));
  logfile.print(gps.time.hour());
  logfile.print(F(":"));
  if (gps.time.minute() < 10) logfile.print(F("0"));
  logfile.print(gps.time.minute());
  logfile.print(F(":"));
  if (gps.time.second() < 10) logfile.print(F("0"));
  logfile.print(gps.time.second());
  logfile.print(F("Z"));
  stream.print(',');
  
  stream.print(String(gps.location.lat(), 6));
  stream.print(',');
  stream.print(String(gps.location.lng(), 6));
  stream.print(',');

  stream.print(temperature, 2);
  stream.print(',');
  stream.print(humidity, 2);
  stream.print(',');
  stream.print(pm25, 2);
  stream.print(',');
  stream.print(pm10, 2);
  stream.print(',');
  stream.println();
}

void setup() {
  // Open serial communications and wait for port to open
  DEBUG_OUT.begin(9600);
  while (!DEBUG_OUT) ;
  
  DEBUG_OUT.print("Initializing sensors...");
  Serial3.begin(4800); // GPS
  Serial.begin(9600);  // SDS011
  Wire.begin();
  HDC.begin(HDC100X_TEMP_HUMI, HDC100X_14BIT, HDC100X_14BIT, DISABLE);
  temperature = HDC.getTemp();
  DEBUG_OUT.println("done!");
  
  if (!check_sdcard(SD_CHIPSELECT)) {
    while (true) ;
  }

  // sd card
  //pinMode(SD_CHIPSELECT, INPUT);
  //digitalWrite(SD_CHIPSELECT, HIGH);

  if (!SD.begin(SD_CHIPSELECT)) {
    DEBUG_OUT.println("can't open SD card, though a card was found...");
    while (true) ;
  }

  DEBUG_OUT.print("starting GPS...");
  while (!updateLocation()) ; // get first fix & keep GPS enabled afterwards
  lastLat = gps.location.lat();
  lastLng = gps.location.lng();

  logfile_path = make_logfile_path();

  DEBUG_OUT.print("got fix. ready to write data to ");
  DEBUG_OUT.println(logfile_path);

  // if the logfile does not exist yet, write the csv header
  if (!SD.exists(logfile_path)) {
    logfile = SD.open(logfile_path, FILE_WRITE);
    if (logfile) {
      logfile.println("timestamp,latitude,longitude,temperature,humidity,pm2.5,pm10");
      logfile.close();
    } else {
      DEBUG_OUT.print("unable to write CSV header to logfile");
      while (true) ;
    }
  }
}

void loop(void) {
  cyclestart = millis();

  if (!updateLocation()) {
    return DEBUG_OUT.println("couldnt get fix");
  }

  lastLat = gps.location.lat();
  lastLng = gps.location.lng();


  //-----Temperature-----//
  temperature = HDC.getTemp();
  delay(100);

  //-----Humidity-----//
  humidity = HDC.getHumi();
  delay(100);

  //-----fine dust-----/
  error = sds.read(&pm25, &pm10);
  if (error) {
    return DEBUG_OUT.println("could not read SDS011...");
  }

  write_log_to_stream(DEBUG_OUT);

  // if the file opened okay write GPS fix to it
  logfile = SD.open(logfile_path, FILE_WRITE);
  if (logfile) {
    DEBUG_OUT.print("writing to logfile ");
    DEBUG_OUT.print(logfile_path);
    DEBUG_OUT.print(" ... ");

    write_log_to_stream(logfile);

    logfile.close();
    DEBUG_OUT.println("done.");
  } else {
    // FIXME: this will never be evaluated, even if the card is removed??
    DEBUG_OUT.println("error opening logfile");
  }

  // adaptive & rollover safe delay, to ensure longrunning & fixed interval
  adaptive_delay(MEASURE_INTERVAL, millis() - cyclestart);
  //delay(MEASURE_INTERVAL);
}

