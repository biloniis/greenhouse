// Module works on wifi kit8 esp8266 https://heltec.org/project/wifi-kit-8/
// Relay module (pin D02)
// Heater - heating cable for floor
// DS18B20 Temperature Sensor (pin D0)
// Used NTP server for synchronise time (to additional light for perspective)
// To control can be used web server. To set temperature range use form on http page. 
//
// библиотека для работы с протоколом 1-Wire
#include <OneWire.h>
// библиотека для работы с датчиком DS18B20
#include <DallasTemperature.h>
#include "heltec.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DTime.h>

// сигнальный провод датчика
#define ONE_WIRE_BUS 0
#define RELAY_PIN 2

#ifndef STASSID
#define STASSID "wifissid"
#define STAPSK  "passwords"
#endif
// создаём объект для работы с библиотекой OneWire
OneWire oneWire(ONE_WIRE_BUS);

// создадим объект для работы с библиотекой DallasTemperature
DallasTemperature sensor(&oneWire);

ESP8266WebServer server(80);

const char * ssid = STASSID; // your network SSID (name)
const char * pass = STAPSK;  // your network password

DTime currentTime;
unsigned int localPort = 2390;      // local port to listen for UDP packets
unsigned int porogTemp = 11;

int hysteresis = 2;
unsigned long timeFromSync = 0;
unsigned long epoch;
unsigned long deltaTime=millis();

/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

float temperature;

boolean isHeaterOn;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Serial Enable*/);
  Heltec.display->setContrast(255);
  // начинаем работу с датчиком
  sensor.begin();
  // устанавливаем разрешение датчика от 9 до 12 бит
  sensor.setResolution(12);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  udp.begin(localPort);
  //MDNS.begin("esp8266");
  server.on("/", handleRoot);
  server.on("/postform/", handleForm);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() {
  temperature = getTemperature();
  String ntpTime = getTime();
  isHeaterOn = relayControl(temperature);
  displayTemp(temperature, ntpTime, isHeaterOn);
  server.handleClient();
  MDNS.update();
  // ждём одну секунду
  delay(1000);
}

String getTime()
{
  unsigned long deltaSync = millis() - timeFromSync;
  if (timeFromSync==0||deltaSync>3600000)
  {
    WiFi.hostByName(ntpServerName, timeServerIP);
    sendNTPpacket(timeServerIP);
    int cb = udp.parsePacket();
    if (cb)
    {
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      timeFromSync = millis();
      //the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      epoch = secsSince1900 - seventyYears;
    }
  }
  else{
    epoch = epoch+((unsigned long)((millis()-deltaTime)/1000));
    deltaTime=millis();
  }
  currentTime.setTimestamp(epoch);
  char dt[20];
  sprintf(dt, "%04d-%02d-%02d %02d:%02d:%02d", currentTime.year, currentTime.month, currentTime.day, currentTime.hour, currentTime.minute, currentTime.second);
  return String(dt);
}

void sendNTPpacket(IPAddress& address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

float getTemperature()
{
  // переменная для хранения температуры
  float temperature;
  // отправляем запрос на измерение температуры
  sensor.requestTemperatures();
  // считываем данные из регистра датчика
  temperature = sensor.getTempCByIndex(0);
}

void displayTemp(float temperature, String ntpTime, boolean isHeat)
{
  Heltec.display->clear();
  Heltec.display->setLogBuffer(3, 30);
  Heltec.display->print("Temp C: ");
  Heltec.display->print(temperature);
  Heltec.display->println(isHeat ? " H On" : " H Off");
  Heltec.display->println(ntpTime);
  Heltec.display->drawLogBuffer(0, 0);
  Heltec.display->display();
}

boolean relayControl(float temperature)
{
  if (temperature > porogTemp)
  {
    digitalWrite(RELAY_PIN, HIGH);
    return false;
  }
  if (temperature < porogTemp - hysteresis)
  {
    digitalWrite(RELAY_PIN, LOW);
    return true;
  }
}

void handleRoot() {
  char temp[800];
  snprintf(temp, 800,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='10'/>\
    <title>Heater</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Winter garden heater condition</h1>\
    <p>Time: %4d-%2d-%2d %02d:%02d:%02d</p>\
    <p>Temperature: %d.%02d &#8451; </p>\
    <p>Temperature range: %d - %d  &#8451;</p>\
    <p>Heater: %s</p>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postform/\">\
    <p>Porog temperature: <input type=\"text\" name=\"porog\" value=\"%d\"> &#8451;</p>\
    <p>Hysteresis:  <input type=\"text\" name=\"hyst\" value=\"%d\"> &#8451;</p>\
    <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>",

           currentTime.year
           , currentTime.month
           , currentTime.day
           , currentTime.hour
           , currentTime.minute
           , currentTime.second
           , (int)temperature
           , (int)(temperature * 100) % 100
           , porogTemp-hysteresis
           , porogTemp
           , isHeaterOn ? "On" : "Off"
           , porogTemp
           , hysteresis
          );
  server.send(200, "text/html", temp);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void handleForm() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      if (server.argName(i) == "porog")
      {
        porogTemp = server.arg(i).toInt();
      }
      if (server.argName(i) == "hyst")
      {
        hysteresis = server.arg(i).toInt();
      }
    }
    server.sendHeader("Location", String("/"), true);
    server.send ( 302, "text/plain", "");
  }
}
