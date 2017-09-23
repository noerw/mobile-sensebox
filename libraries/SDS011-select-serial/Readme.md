# SDS011 library selectable serialport

Arduino library for dust Sensor SDS011 (Nova Fitness Co.,Ltd) which allows to choose the serial port used to communicate with the sensor.
Based on the work of [ricky-z](https://github.com/ricki-z/SDS011)

## Usage

* Define SDS object:
```
SDS011(Stream& serial);
```
i.e. SDS011 mySDS(Serial1);

* Start serial communication:
```
Serial1.begin(9600);
```

* Read values:
```
int read(float *p25, float *p10);
```
i.e. error = mySDS(pm25,pm10);

Reads the PM2.5 and PM10 values, return code is 0, if new values were read, and 1 if there were no new values.

