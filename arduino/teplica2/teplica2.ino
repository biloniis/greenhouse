/**
 * Temperature sensors
 * T1 - red   - { 0x28, 0xAA, 0x28, 0x0F, 0x46, 0x14, 0x1, 0xB8 }
 * T2 - green - { 0x28, 0xAA, 0xEA, 0x1C, 0x46, 0x14, 0x1, 0xA9 }
 * T3 - blue  - { 0x28, 0xAA, 0x6, 0xF0, 0x45, 0x14, 0x1, 0xED }
 * Sensors data pin - D12 
 * 
 * Relay module 
 * K1 - ~220V - down  - D8
 * K2 - ~220V - up    - D9
 * K3 - +12V  - up    - D10
 * k4 - +12V  - down  - D11
 * 
 * Real time clock A4, A5
 * 
 * Display(current date-month hh:mm(t=DD-MM hh:mm - 13), mode light(l=on/off - 5), mode heater1 (h1=on/off - 6), mode heater2 (h1=on/off - 6), t1 , t2, t external)
 * 2x16
 *   1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
 * 1 D D - M M h h : m m   + t 0  
 * 2 L   + t 1   + t 2     H 1 H 2
 * 
 * Used libraries
 * RTClib.h - real time
 * DallasTemperature.h - temperature sensor
 * OneWire.h - one wire sensors
 * LiquidCrystal_I2C.h - lcd display by i2c bus
 * Wire.h
 */

// Include the libraries we need
#include "OneWire.h"
#include "DallasTemperature.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <PinButton.h>

// Address constants
#define ONE_WIRE_BUS 12
#define LED_PIN_UP 10
#define LED_PIN_DOWN 11
#define HEATER_PIN_UP 9
#define HEATER_PIN_DOWN 8

#define MAY_MORNING 6
#define MAY_EVENING 20

#define MIN_TEMP 9
#define MAX_TEMP 15

#define PIN_button_SET  2; 
#define PIN_button_UP   3;
#define PIN_button_DOWN 4; 
//      uint8_t VAR_mode_SHOW   = 1;  // mode for show: 1-time+out temp 2-state+inner temp
      uint8_t VAR_mode_SET    = 0;  // mode for set: 0 none 1-day 2-month 3-year 4-hour 5-minutes
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
 
DeviceAddress sensor1 = { 0x28, 0xAA, 0x28, 0x0F, 0x46, 0x14, 0x1, 0xB8 }; //red
DeviceAddress sensor2 = { 0x28, 0xAA, 0xEA, 0x1C, 0x46, 0x14, 0x1, 0xA9 }; // green
DeviceAddress sensor3= { 0x28, 0xAA, 0x6, 0xF0, 0x45, 0x14, 0x1, 0xED }; //blue outdoor
LiquidCrystal_I2C lcd(0x27,16,2);  // display settings
RTC_DS1307 rtc; //real time settings

// temperature values
int t1,t2, t3;
uint8_t dd, mm, hh, mi;
uint16_t yy;
boolean l = false;
boolean h1 = false;
boolean h2 = false;
void setup() {
  lcd.init();
  lcd.backlight();
  pinMode(LED_PIN_UP, OUTPUT);
  pinMode(LED_PIN_DOWN, OUTPUT);
  pinMode(HEATER_PIN_UP, OUTPUT);
  pinMode(HEATER_PIN_DOWN, OUTPUT);
  pinMode(PIN_button_SET,  INPUT);
  pinMode(PIN_button_UP,   INPUT);
  pinMode(PIN_button_DOWN, INPUT);
  Wire.begin();
  Serial.begin(9600);
  rtc.begin();
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // строка ниже используется для настройки даты и времени часов
    // RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  sensors.begin();
  
}

void loop() {
  if(millis()%500==0)
  {
    getTemperature();
    printTimeTemp(getTime());
    processLight();
    processHeater();
  }
  buttonsControl();
  delay(1);
}

void buttonsControl()
{
  uint8_t i=0;
  DateTime t = getTime();
  if(VAR_mode_SET)
  {
      if(!digitalRead(PIN_button_UP))
      {
        while(!digitalRead(PIN_button_UP))
        {
          delay(50);
        }                // ждём пока мы не отпустим кнопку UP
        Serial.println("PIN_button_UP heated. VAR_mode_SET="||VAR_mode_SET);
        switch (VAR_mode_SET)
        {                                     // инкремент (увеличение) устанавливаемого значения
          /* day */   case 1: dd=(dd==31)?1:dd++; break;
          /* month */ case 2: mm=(mm==12)?1:mm++; break;
          /* year */  case 3: yy=yy++; break;
          /* hour */  case 4: hh=(hh==23)?0:hh++; break;
          /* min */   case 5: mi=(mi==59)?0:mi++; break;
        }
        //t = DateTime(yy,mm,dd,hh,mi,0);
      }
      if(!digitalRead(PIN_button_DOWN))
      {
        while(!digitalRead(PIN_button_DOWN))
        {
          delay(50);
        }                // ждём пока мы не отпустим кнопку UP
        Serial.println("PIN_button_DOWN heated. VAR_mode_SET="||VAR_mode_SET);
        switch (VAR_mode_SET)
        {                                     // инкремент (увеличение) устанавливаемого значения
          /* day */   case 1: dd=(dd==1)?31:dd--; break;
          /* month */ case 2: mm=(mm==1)?12:mm--; break;
          /* year */  case 3: yy=yy--; break;
          /* hour */  case 4: hh=(hh==0)?23:hh--; break;
          /* min */   case 5: mi=(mi==0)?59:mi--; break;
        }
        //t = new DateTime(yy,mm,dd,hh,mi,0);
      }
      rtc.adjust(DateTime(yy,mm,dd,hh,mi,0));
      if(!digitalRead(PIN_button_SET))
      {
        while(!digitalRead(PIN_button_SET))
        {                          // ждём пока мы её не отпустим
          delay(10);
          if(i<200)
          {
            i++;
          }
          else
          {
            Serial.println("set hitted more then 2 secs.");
            lcd.clear();
          }                          // фиксируем, как долго удерживается кнопка SET, если дольше 2 секунд, то стираем экран
      }
      if(i<200){                                                 // если кнопка SET удерживалась меньше 2 секунд
          VAR_mode_SET++;                                          // переходим к следующему устанавливаемому параметру
          if(VAR_mode_SET>5){VAR_mode_SET=1;}  // возвращаемся к первому устанавливаемому параметру
      }else{                                                     // если кнопка SET удерживалась дольше 2 секунд, то требуется выйти из режима установки даты/времени
          VAR_mode_SET=0;                                          // выходим из режима установки даты/времени
      }
    }
  }
  else
  {
      if(!digitalRead(PIN_button_SET)){
      while(!digitalRead(PIN_button_SET))
      {
        delay(10);                // ждём пока мы её не отпустим
        if(i<200)
        {
          i++;
        }else
        {
          lcd.clear();
        }                          // фиксируем, как долго удерживается кнопка SET, если дольше 2 секунд, то стираем экран
      }
        VAR_mode_SET=1;
      }
    }
}

void printTimeTemp(DateTime t)
{
  char lineOne[16];
  char lineTwo[16];
  if(VAR_mode_SET==0)
  {
    lcd.setCursor(0, 0);
    sprintf(lineOne, "%02d-%02d %02d:%02d %s%02d", dd,mm,hh,mi,t3>=0?"+":"-",t3);
    sprintf(lineTwo, "%s %s%02d %s%02d %2s %2s", l?"L":" ",t1>=0?"+":"-",t1, t2>=0?"+":"-",t2, h1?"H1":"  ", h2?"H2":"  ");//, h1, h2);
    lcd.print(String(lineOne));
    lcd.setCursor(0, 1);
    lcd.print(String(lineTwo));
    Serial.println(String(lineOne)+' '+String(lineTwo));
  }
  else
  {
    Serial.println("Mode:");Serial.print(VAR_mode_SET);
    lcd.setCursor(0,0);
    if(VAR_mode_SET==1){lcd.print("Set day: "); lcd.print(dd);}
    if(VAR_mode_SET==2){lcd.print("Set month: ");lcd.print(mm);}
    if(VAR_mode_SET==3){lcd.print("Set year: ");lcd.print(yy);}
    if(VAR_mode_SET==4){lcd.print("Set hour: ");lcd.print(hh);}
    if(VAR_mode_SET==5){lcd.print("Set minutes: ");lcd.print(mi);}
  }
}

void processHeater()
{
  if(t1<MIN_TEMP)
  {
    h1=true;
  }
  else if(t1>MAX_TEMP)
  {
    h1=false;   
  }
  heater(HEATER_PIN_UP, h1);
  if(t2<MIN_TEMP)
  {
    h2=true;
  }
  else if(t2>MAX_TEMP)
  {
    h2=false;
  }
  heater(HEATER_PIN_DOWN, h2);
}

void heater(int pin, boolean isHeaterOn)
{
  digitalWrite(pin, isHeaterOn ? LOW : HIGH);
}

void processLight()
{
  if(hh>=MAY_MORNING && hh<getMorningTime())
  {
    l=true;
  }
  else if(hh>=getEveningTime() && hh<MAY_EVENING)
  {
    l=true;
  }
  else
  {
    l=false;
  } 
  light(l);
}

int getMorningTime()
{
  if(mm==10 || mm==3 || mm==4)
  {
    return 7;
  }
  else if(mm==11 || mm==2)
  {
    return 8;
  }
  else if(mm==12 || mm==1)
  {
    return 9;
  }
  
}

int getEveningTime()
{
  if(mm==10 || mm==3 || mm==4)
  {
    return 19;
  }
  else if(mm==11 || mm==2)
  {
    return 18;
  }
  else if(mm==12 || mm==1)
  {
    return 17;
  }
  
}

void light(boolean isLight)
{
  if(isLight)
  {
    Serial.println("Light on!");
    digitalWrite(LED_PIN_UP, LOW);
    digitalWrite(LED_PIN_DOWN, LOW);
  }
  else
  {
    Serial.println("Light off!");
    digitalWrite(LED_PIN_UP, HIGH);
    digitalWrite(LED_PIN_DOWN, HIGH);
  }
}

void getTemperature()
{ 
  sensors.requestTemperatures();
  t1=sensors.getTempC(sensor1);
  t2=sensors.getTempC(sensor2);
  t3=sensors.getTempC(sensor3);
}

DateTime getTime()
{
  DateTime now = rtc.now();
  mm=now.month();
  dd=now.day();
  hh=now.hour();
  mi=now.minute();
  yy=now.year();
  return now;
}

