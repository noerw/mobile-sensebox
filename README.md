# mobile senseBox

This is some Arduino code for GPS tracked senseBoxes.
They have an SDS011 particulate matter sensor & HDC1008 Temp & Humidity sensor
attached and transmit their measurements to opensensemap.org.

There are multiple variants to make use of various hardware I had available, and
to evaluate different use cases and workflows of data transmission.

Each sketch folder has its own readme with (more or less) detailed documentation.
The `libraries` folder contains all code that any sketch may depend on.
The the most simple solution to use these sketches is to make a backup of your
existing `~/Arduino` folder, and clone this repository there instead:

```sh
mv ~/Arduino ~/Arduino.bak
git clone https://github.com/noerw/mobile-sensebox ~/Arduino
```

## Hardware
The microcontroller & communication protocol differs for the sketches, but all
variants require a GPS device, a microcontroller with I2C, 2 UARTs (or some sort
of software UART), as well as an Novafit SDS011 & a Bosch HDC1008.

My builds of all these setups are powered by a 5.6Ah LiPo through an Adafruit
LiPo charger via USB, and enclosed in a 15x8x8 case.
The humidity sensor is included for reference, as the SDS011 only provides valid
data at low humidity. The sensor is enclosed within the case (which heats up
quite a bit), which makes the measurements quite inaccurate, they should only be
treated as approximations.
This setup is quite suboptimal for regular deployment (eg. daily commute by
bike), see `Future Work`.

### `./lora-gps`
Arduino Mega + Dragino LoRa & GPS Shield + senseBox Shield (SD card)
- transmitting data via LoRa to opensensemap.org through thethingsnetwork.org (details in SETUP.md)
- additionally saving to SD card if no LoRa coverage is available
- Arduino Uno should work as well, you need to play around with SoftSerial i guess..

### `./esp8266-gps`
Wemos D1 (ESP8266) + senseBox Shield
- stores measurements locally on 3MB SPIFFS, and uploads via WiFi when available
- requires installation of ESP8266 Arduino SDK, see `esp8266-gps/README.md`
- senseBox shield only used for ease of use with JST-connectors, not needed

### `./sdcard-gps`
Arduino Mega + senseBox Shield
- logging to SD card, manual upload to opensensemap.org

## Future Work
- Evaluation of SDS011 in a mobile environment: Influence of wind, sunlight,
  vibrations, differing orientations

- Averaging of measurement values
    - tradeoff: less outliers <-> smaller spatial acurracy

- Improved case
    - power switch on the outside
    - simple, quick mounting on a bike
    - status LEDs about connectivity, GPS
    - more neutral placement of humidity sensor
    - all-weather proof

# License
- sketch directories:      MIT Norwin Roosen
- `libraries` directory: see each subdirectory


