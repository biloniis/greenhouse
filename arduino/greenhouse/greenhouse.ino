#include "DHT.h"
#include "iarduino_RTC.h"

#define DHTPIN 2     // what digital pin we're connected to

#define DHTTYPE DHT11   // DHT 11
#define HEATER_PIN 7 //pin for heater reley module
#define LIGHT_PIN 3 //pin for light relay module

iarduino_RTC time(RTC_DS1307);
DHT dht(DHTPIN, DHTTYPE);
//Application constants and variables
const float MIN_TEMP=14.0; // temperature for switch on heater 
const float MAX_TEMP=18.0; // temperature for switch off heater

bool isHeaterOn;
bool isLightOn;

void setup() {
  Serial.begin(9600);
  Serial.println("DHTxx test!");
	pinMode(HEATER_PIN, OUTPUT);
	digitalWrite(HEATER_PIN, HIGH);
	isHeaterOn = false;
	pinMode(LIGHT_PIN, OUTPUT);
	digitalWrite(LIGHT_PIN, HIGH);
	isLightOn = true;
	time.begin();
  dht.begin();
	time.settime(0,16,16,28,10,18,7);
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
  Serial.println(t);
	if(t < MIN_TEMP && !isHeaterOn)
	{
		Serial.println("Heater on!");
		isHeaterOn = true;
		digitalWrite(HEATER_PIN, LOW);
	}
	if(t > MAX_TEMP && isHeaterOn)
	{
		Serial.println("Heater off!");
		isHeaterOn = false;	
		digitalWrite(HEATER_PIN, HIGH);
	}
	Serial.println(time.gettime("d-m-Y, H:i:s, D"));
  time.gettime();
  int hours = time.Hours;
  Serial.println(hours);
	if( (hours >= 17 && hours < 22 && !isLightOn) ||  ( hours > 5 && hours <= 8 && !isLightOn ))
	{
		Serial.println("Light on!");
		isLightOn = true;
		digitalWrite(LIGHT_PIN, LOW);
	}
	if( (hours >= 22 ||(hours > 8 && hours < 17)) && isLightOn )
	{
		Serial.println("Light off!");
		isLightOn = false;
		digitalWrite(LIGHT_PIN, HIGH);
	}
}
