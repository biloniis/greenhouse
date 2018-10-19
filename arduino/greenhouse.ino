#include "DHT.h"
#include "iarduino_RTC.h"

#define DHTPIN 2     // what digital pin we're connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
iarduino_RTC time(RTC_DS1307);
DHT dht(DHTPIN, DHTTYPE);
//Application constants and variables
#define MIN_TEMP 14 // temperature for switch on heater 
#define MAX_TEMP 18 // temperature for switch off heater
#define HEATER_PIN 3 //pin for heater reley module
#define LIGHT_PIN 7 //pin for light relay module
boolean isHeaterOn;
boolean isLightOn;

void setup() {
  Serial.begin(9600);
  Serial.println("DHTxx test!");
	pinMode(HEATER_PIN, OUTPUT);
	digitalWrite(HEATER_PIN, LOW);
	isHeaterOn = false;
	pinMode(LIGHT_PIN, OUTPUT);
	digitalWrite(LIGHT_PIN, LOW);
	isLightOn = false;
	time.begin();
  dht.begin();
	//time.settime(0,51,21,20,10,18,6);
}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
	if(t < MIN_TEMP && !isHeaterOn)
	{
		Serial.println("Heater on!");
		isHeaterOn = true;
		digitalWrite(HEATER_PIN, HIGH);
	}
	if(t > MAX_TEMP && isHeaterOn)
	{
		Serial.println("Heater off!");
		isHeaterOn = false;	
		digitalWrite(HEATER_PIN, LOW);
	}
	int hours = time.getTime("H");
	if( hours > 17 && hours <22 && !isLightOn )
	{
		Serial.println("Light on!");
		isLightOn = true;
		digitalWrite(LIGHT_PIN, HIGH);
	}
	if( hours > 5 && hours < 8 && !isLightOn )
	{
		Serial.println("Light on!");
		isLightOn = true;
		digitalWrite(LIGHT_PIN, HIGH);
	}
	if( hours > 22 && isLightOn )
	{
		Serial.println("Light off!");
		isLightOn = false;
		digitalWrite(LIGHT_PIN, LOW);
	}
	if( hours > 8 && hours < 17 && isLightOn )
	{
		Serial.println("Light off!");
		isLightOn = false;
		digitalWrite(LIGHT_PIN, LOW);
	}
}
 