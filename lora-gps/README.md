# lora-gps senseBox

This is a GPS tracked senseBox measuring particulate matter concentrations,
transmitting its data via LoRaWAN through [TheThingsNetwork] to [openSenseMap].

[TheThingsNetwork]: thethingsnetwork.org
[openSenseMap]: opensensemap.org

It's based on
- Arduino Mega (Arduino Uno should work to, just use SoftwareSerial for the SDS011)
- Dragino LoRa- & GPS-Shield
- SD-card reader (I used the one on the senseBox-Shield. Any other one should work as well, change the `SD_PIN` in `config.h`)
- Novafit SDS011 particulate matter sensor
- HDC1008 temp- & humidity sensor

For mobile power supply I used an Adafruit LiPo charger + 5.6Ah LiPo.

## Notes about device operation
The device only measures and transmits once there was an initial GPS fix and a
successful handshake with TheThingsNetwork. This means the device only begins
operating when placed outdoors in areas with [TTN coverage](https://ttnmapper.org).

**Important**: TheThingsNetwork has a fair use policy, giving each device an
uplink transmission time of 300 seconds per day. This means, that the rate at
which you transmit depends on the duration you use the device per day.
Each message payload has 24 bytes and is sent on a slow datarate SF10 (for higher range).
You can use [this calculator](https://docs.google.com/spreadsheets/d/1eL1nHxMidIcIdDE_l-DoY3kmE2e8b0YpOBW64WnYxj8)
to find a transmission interval that suits you.

## Sketch setup & opsensensemap integration

- Register a device under <https://console.thethingsnetwork.org>
  - register a new application first
  - register a new device
  - note down the following values: Device ID, Application ID, Device EUI, App EUI, App Session Key
  - add the HTTP Integration to your application and point it to POST to `https://ttn.opensensemap.org/v1.1`
    (no authentication headers are required)
- Register a senseBox at <https://opensensemap.org/register> (or use the script `register-osem.sh`!)
  - select `mobile` for exposure
  - select the location where you plan the first deployment
  - select the model `Luftdaten with SDS011 & DHT22`
  - enable TheThingsNetwork Integration, and insert your Device ID and AppId, choose profile `lora-serialization`.
    Insert the following into Decode Options:
    ```json
    [{"decoder":"latLng"},{"decoder":"temperature","sensor_title":"Temperatur"},{"decoder":"humidity","sensor_title":"rel. Luftfeuchte"},{"decoder":"temperature","sensor_title":"PM2.5"},{"decoder":"temperature","sensor_title":"PM10"}]
    ```

- Insert the EUIs & App Session Key you received from TTN in `config.h`

## Hardware Setup
- Flash the sketch using Arduino IDE
- Stack the senseBox Shield onto the Arduino, then stack the Dragino shield on top.
- Remove the `GPS_TX` and `GPS_RX` jumpers on the Dragino Shield, and wire these
  pins instead to the Arduinos `RX2` and `TX2` pins.
- Wire the SDS011: RX & TX go to `RX3` and `TX3` on the Arduino, GND to GND, 5V to 5V ;)
- Connect the LiPo to the LiPo-Charger, and connect the USB Output to the Arduino
- Done!
