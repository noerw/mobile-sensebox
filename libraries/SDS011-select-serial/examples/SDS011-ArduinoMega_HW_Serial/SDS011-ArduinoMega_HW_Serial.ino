// SDS011 dust sensor example
// for use with additional Serial ports
// like Arduino Mega
// -----------------------------

#include <SDS011-select-serial.h>

float p10,p25;
int error;

SDS011 my_sds(Serial1);

void setup() {
	// initialize normal Serial port
	Serial.begin(9600);
	// initalize SDS Serial Port
	Serial1.begin(9600);
}

void loop() {
	error = my_sds.read(&p25,&p10);
	if (! error) {
		Serial.println("P2.5: "+String(p25));
		Serial.println("P10:  "+String(p10));
	}
	delay(100);
}
