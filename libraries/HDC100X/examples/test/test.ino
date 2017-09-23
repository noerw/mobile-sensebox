#include <Wire.h>
#include <HDC100X.h>

HDC100X hdc;

#define LED 13
bool state = false;

void setup()
{
  Serial.begin(9600);

  hdc.begin(HDC100X_TEMP_HUMI,HDC100X_14BIT,HDC100X_14BIT,DISABLE);
}
 
void loop()
{
  Serial.print(" Humidity: ");
  Serial.print(hdc.getHumi()); 
  Serial.print("%, Temperature: ");     
  Serial.print(hdc.getTemp());
  Serial.println("C");

  delay(500);
}
