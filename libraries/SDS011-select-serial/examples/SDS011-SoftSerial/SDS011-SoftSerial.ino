// SDS011 dust sensor example
// for use with SoftSerial
// -----------------------------

#include <SDS011-select-serial.h>
#include <SoftwareSerial.h>

float p10,p25;
int error;

SoftwareSerial mySerial(10, 11); // RX, TX
SDS011 my_sds(mySerial);

void setup() {
	// initialize normal Serial port
	Serial.begin(9600);
	// initalize SDS Serial Port
	mySerial.begin(9600);
}

void loop() {
	error = my_sds.read(&p25,&p10);
	if (! error) {
		Serial.println("P2.5: "+String(p25));
		Serial.println("P10:  "+String(p10));
	}
	delay(100);
}
