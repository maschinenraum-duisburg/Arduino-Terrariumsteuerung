#include <Wire.h>                             //I2C-Schnittstelle
#include "SSD1306AsciiWire.h"                 //<-OLED
#define I2C_ADDRESS 0x3C         
#define OLED_WIDTH 128
#define OLED_HEIGHT 64   
SSD1306AsciiWire oled;                        //OLED->

#include <DHT.h>                              //<-Sensor *evtl. weitere Sensoren hinzufügen
#define DHTTYPE DHT11    
#define DHTPIN 2                              
DHT dht(DHTPIN, DHTTYPE);                     //Sensor->

#include <virtuabotixRTC.h>                           //<-Realtime Clockmodul  
virtuabotixRTC myRTC(9, 10, 11);                        // CLK,DAT,RST Clockmodul->
//----------------------------------------------------------------------------//Variablen
int dauer1 = 2000;                         //Zeitabstand Timerschleife
int idleTimer = 5000;                         //Zeitabstand Timerschleife
unsigned long timer1 = 0;                       //Initialwert
unsigned long lastAction = 0;                       //Initialwert
                                
int buttonPressed = 0;                              //<-Taster Initialwert
const int btns[] =  {6,5,8,7,4};                     //Tasterpins
const String btnsVal[] =  {"dwn","up","lft","rgt","ok"}; //TasterWerte für leichtere Lesbarkeit
const int btnCount = sizeof(btns)/sizeof(btns[0]);  //Anzahl Taster
int lastBtnState[btnCount];                         //Array zum abspeichern des letzte Tasterzustands Taster->

int page = 1;                                  //"Seite" der Anzeige
int maxScreen = 3;                             //max Anzahl der Anzeigen
int menuItem = 0;                               //Menüeintrag Startwert 0=IdleScreen
int maxMenuItems = 7;                           //Anzahl der Menüeinträge
int settings = 0;                                //status ob settings geöffnet ist oder nicht
int maxSubmenu[3] = {5,2,2};                      //Anzahl der Submenus pro MenuEintrag
int submenu = 0;                                  //Submenu Startwert 0=keins!
                   
int hour = (__TIME__[0] - '0') * 10 + (__TIME__[1] - '0');//<-Einstellungen(defaultWerte)
int minute = (__TIME__[3] - '0') * 10 + (__TIME__[4] - '0');
int day = ((__DATE__[4] >= '0' && __DATE__[4] <= '9') ? (__DATE__[4] - '0') * 10 : 0) + (__DATE__[5] - '0');
int month;
int year = ((__DATE__[7] - '0') * 10 + (__DATE__[8] - '0')) * 100 + (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');
  
int dayTime[2] = {7,00};                              //Standardwerte für Uhrzeit Tag/Nacht
int nightTime[2] = {19,00};

float dayTemp = 32.0;                                 //Standardwerte für Temperatur Tag/Nacht
float nightTemp = 25.0;
float tempTolerance = 2;                              //toleranz für Temperatur

float dayHum = 65.0;                                  //Standardwerte für Feuchtigkeit Tag/Nacht
float nightHum = 80.0;
float humTolerance = 8;                               //toleranz für Feuchtigkeit

bool light = 0;                                       //Variablen für Steuerung der Relais
bool heat = 0;                                        
bool humidity = 0;                                    

//-----------------------------------------------------------------------------//Setup
void setup() {
  Serial.begin(9600);                         //Serialverbindung für Debugging 

  dht.begin();                                //Sensor
  
  Wire.begin();                               //<-OLED
  Wire.setClock(5000L);
  oled.begin(&Adafruit128x64, I2C_ADDRESS);   
  oled.setFont(fixed_bold10x15);              //OLED->
    
  for (int i = 0; i < btnCount; i++) {        //<-Taster 
    pinMode(btns[i], INPUT);                  //Alle Tastereingänge festlegen und den letzen Status auf LOW setzen(initialwert)
    lastBtnState[i] = LOW;                    //Taster->
  }

  pinMode(A1, OUTPUT);               //Pins als Output festlegen(Relais)
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT); 
//Pins Setup->
  
  switch (__DATE__[0]) {
    case 'J':
      if (__DATE__[1] == 'a') {
        month = 1;
      } else if (__DATE__[2] == 'n') {
        month = 6;
      } else {
        month = 7;
      }
      break;
    case 'F':
      month = 2;
      break;
    case 'A':
      month = (__DATE__[1] == 'p') ? 4 : 8;
      break;
    case 'M':
      month = (__DATE__[2] == 'r') ? 3 : 5;
      break;
    case 'S':
      month = 9;
      break;
    case 'O':
      month = 10;
      break;
    case 'N':
      month = 11;
      break;
    case 'D':
      month = 12;
      break;
    default:
      month = 1;
      break;
  }                  
  myRTC.setDS1302Time(0, minute, hour, 0, day, month, year);
  myRTC.updateTime();
}

//----------------------------------------------------------------------------//FUNKTIONEN
float readSensor(int choice)                  //Auswahl des Ergebnistyps         
{     
  oled.set2X();
  switch (choice) {                                      
    case 1:                                   //Sensor Temperatur
      return dht.readTemperature();
      break;
    case 2:                                   //Sensor Feuchtigkeit
      return dht.readHumidity();
      break;
  }
}

String getMode()                            //Auswertung Tag-/ Nachtzeit
{
  if(myRTC.hours == dayTime[0]){
    if(myRTC.minutes >= dayTime[1]){
      return "Tag";
    }
    return "Nacht";
  }
  if(myRTC.hours == nightTime[0]){
    if(myRTC.minutes >= nightTime[1]){
      return "Nacht";
    }
    return "Tag";
  }
  if(myRTC.hours > dayTime[0] && myRTC.hours < nightTime[0]){
    return "Tag";
  }
  if(myRTC.hours < dayTime[0] || myRTC.hours > nightTime[0]){
    return "Nacht";
  }
  return "Invalid"; 
}
void showTime()                               //zeigt Zeit und Datum an
{
  myRTC.updateTime();
  oled.set1X();
  if(myRTC.dayofmonth < 10){ oled.print("0");}  
  oled.print(myRTC.dayofmonth);                                                                        
  oled.print(".");
  if(myRTC.month < 10){ oled.print("0");}                                                                                        
  oled.print(myRTC.month);                                                                             
  oled.print(".");                                                                                      
  oled.println(myRTC.year);                                                                               
  oled.set2X();         
  if(myRTC.hours < 10){ oled.print("0");}                                                                           
  oled.print(myRTC.hours);                                                                              
  oled.print(":");            
  if(myRTC.minutes < 10){ oled.print("0");}                                                                          
  oled.println(myRTC.minutes);
  oled.set1X();                                                                        
}

void mainScreen(int choice){              //Ausgeben der Sensordaten aus OLED (IdleScreen) 
  oled.clear();
  oled.set1X();
  switch(choice){
    case 1:
      oled.println("Temperatur");
      oled.println(readSensor(1));
      oled.set1X();
      break;
    case 2: 
      oled.println("Feuchtigkeit");
      oled.println(readSensor(2));
      oled.set1X();
      break;
    case 3: 
      showTime();
      oled.set1X();
      break;
  }
}
void setValues(){                                         //Schalten der Relais
  if(light == 1)
  {
    digitalWrite(A3, HIGH);
  }
  else
  {
    digitalWrite(A3, LOW);
  }
  if(humidity == 1)
  {
    digitalWrite(A2, HIGH);
  }
  else
  {
    digitalWrite(A2, LOW);
  }
  if(heat == 1)
  {
    digitalWrite(A1, HIGH);
  }
  else
  {
    digitalWrite(A1, LOW);
  }
}

void checkSettings(){                                         //Auswertung Sensorwerte und Einstellungen für die Tag- /Nachtzeit & setzen der Werte für die Relais-Steuerung
  if(getMode() == "Tag"){
    light = 1;
    if(readSensor(1) - tempTolerance < dayTemp ){
      heat = 1;
    }
    else if (readSensor(1) >= dayTemp){
      heat = 0;
    }
    if(readSensor(2) - humTolerance < dayHum){
      humidity = 1;
    }
    else if (readSensor(2) >= dayHum){
      humidity = 0;
    }
  }
  else if(getMode() == "Nacht"){                
    light = 0;
    if(readSensor(1) - tempTolerance < nightTemp){
      heat = 1;
    }
    else if (readSensor(1) >= nightTemp){
      heat = 0;
    }
    if(readSensor(2) - tempTolerance < nightHum){
      humidity = 1;
    }
    else if (readSensor(2) >= nightHum){
      humidity = 0;
    }
  }
  setValues();
}

int getButton(int PR) {                                           //Tasterdruckabfrage mit Entprellung PR = 0 für Release und 1 für Druck
  static unsigned long lastButtonPressTime = 0;
  const unsigned long debounceDelay = 200; 

  for (int i = 0; i < btnCount; i++) {
    if (digitalRead(btns[i]) == HIGH && lastBtnState[i] == LOW) {
      if (millis() - lastButtonPressTime >= debounceDelay) { 
        lastBtnState[i] = HIGH;
        if (PR == 1) {
          lastButtonPressTime = millis(); 
          return i;
        }
      }
    } 
    else if (digitalRead(btns[i]) == LOW && lastBtnState[i] == HIGH) {
      if (millis() - lastButtonPressTime >= debounceDelay) { 
        lastBtnState[i] = LOW;
        if (PR == 0) {
          lastButtonPressTime = millis();
          return i;
        }
      }
    }
  }
  return -1;
}

String getButtonVal(int strPosition) {      //gibt Wert von Taster zurück
  return btnsVal[strPosition];
}

void showMenu(int menuItem) {                  //Menüstruktur (steuert menu menuposition= position im menu, selection=gedrückter Taster)
  switch(menuItem) {
    case 1:  
      oled.clear();
      oled.set1X();
      oled.println("Uhrzeit");
      showTime();
      break;
    case 2: 
      oled.clear();
      oled.set1X();
      oled.println("Tagzeit");
      oled.set2X();
      if(dayTime[0] < 10){ oled.print("0");}  
      oled.print(dayTime[0]);
      oled.print(":");
      if(dayTime[1] < 10){ oled.print("0");}  
      oled.println(dayTime[1]);
      break;
    case 3:  
      oled.clear();
      oled.set1X();
      oled.println("Nachtzeit");
      oled.set2X();
      if(nightTime[0] < 10){ oled.print("0");}
      oled.print(nightTime[0]);
      oled.print(":");
      if(nightTime[1] < 10){ oled.print("0");}
      oled.println(nightTime[1]);
      break;
    case 4: 
      oled.clear();
      oled.set1X();
      oled.println("Temperatur");
      oled.println("Tag");
      oled.set2X();
      oled.println(dayTemp);
      break;    
    case 5:  
      oled.clear();
      oled.set1X();
      oled.println("Temperatur");
      oled.println("Nacht");
      oled.set2X();
      oled.println(nightTemp);
      break;
    case 6: 
      oled.clear();
      oled.set1X();
      oled.println("Feuchtigkeit");
      oled.println("Tag");
      oled.set2X();
      oled.println(dayHum);
      break;
    case 7: 
      oled.clear();
      oled.set1X();
      oled.println("Feuchtigkeit");
      oled.println("Nacht");
      oled.set2X();
      oled.println(nightHum);
      break;
    case 8: 
      oled.clear();
      oled.set1X();
      oled.println("Menue");
      oled.println("schliessen");
      break;
  }
}
void changeValue(int selection, float value)      //Einstellungen ändern
{
  switch(selection)
  {
    case 4: 
      dayTemp += value;
      break;    
    case 5:  
      nightTemp += value;
      break;
    case 6: 
      dayHum += value;
      break;
    case 7: 
      nightHum += value;
      break;
  }
  showMenu(selection);
}

void showSetting(int selection)           //Zeit-Einstellungen anzeigen
{
  switch(selection)
  {
    case 1:
      oled.clear();
      oled.set1X();
      oled.println("Einstellungen");
      oled.println("verlassen");
      break;
    case 11:
      oled.clear();
      oled.set1X();
      oled.println("   Tag   ");
      oled.println("einstellen");
      oled.set2X();
      oled.println(myRTC.dayofmonth);
      break;
    case 12:
      oled.clear();
      oled.set1X();
      oled.println("  Monat   ");
      oled.println("einstellen");
      oled.set2X();
      oled.println(myRTC.month);
      break;
    case 13:
      oled.clear();
      oled.set1X();
      oled.println("  Jahr   ");
      oled.println("einstellen");
      oled.set2X();
      oled.println(myRTC.year);
      break;
    case 14:
      oled.clear();
      oled.set1X();
      oled.println("Uhrzeit");
      oled.println("einstellen");
      oled.set2X();
      if(myRTC.hours < 10){ oled.print("0");}
      oled.print(myRTC.hours);
      oled.print(":");
      oled.set1X();
      if(myRTC.minutes < 10){ oled.print("0");}
      oled.println(myRTC.minutes);
      break;    
    case 15:
      oled.clear();
      oled.set1X();
      oled.println("Uhrzeit");
      oled.println("einstellen");
      if(myRTC.hours < 10){ oled.print("0");}
      oled.print(myRTC.hours);
      oled.set2X();
      oled.print(":");
      if(myRTC.minutes < 10){ oled.print("0");}
      oled.println(myRTC.minutes);
      break;
    case 21:
      oled.clear();
      oled.set1X();
      oled.println("Tagzeit");
      oled.println("Stunden");
      oled.set2X();
      if(dayTime[0] < 10){ oled.print("0");}  
      oled.print(dayTime[0]);
      oled.print(":");
      if(dayTime[1] < 10){ oled.print("0");}  
      oled.println(dayTime[1]);
      break;
    case 22:
      oled.clear();
      oled.set1X();
      oled.println("Tagzeit");
      oled.println("Minuten");
      oled.set2X();
      if(dayTime[0] < 10){ oled.print("0");}  
      oled.print(dayTime[0]);
      oled.print(":");
      if(dayTime[1] < 10){ oled.print("0");}  
      oled.println(dayTime[1]);
      break;
    case 31:
      oled.clear();
      oled.set1X();
      oled.println("Nachtzeit");
      oled.println("Stunden");
      oled.set2X();
      if(nightTime[0] < 10){ oled.print("0");}
      oled.print(nightTime[0]);
      oled.print(":");
      if(nightTime[1] < 10){ oled.print("0");}
      oled.println(nightTime[1]);
      break;
    case 32:
      oled.clear();
      oled.set1X();
      oled.println("Nachtzeit");
      oled.println("Minuten");
      oled.set2X();
      if(nightTime[0] < 10){ oled.print("0");}
      oled.print(nightTime[0]);
      oled.print(":");
      if(nightTime[1] < 10){ oled.print("0");}
      oled.println(nightTime[1]);
      break;
  }
}

void changeSetting(int selection, int value)          //ändern der Zeit-Einstellungen
{
  switch(selection)
  {
    case 11:
      if(day > 1 && day < 31 )
      {
        day += value;
      }
      break;
    case 12:
      if(month > 1 && month < 31 )
      {
        month += value;
      }
      break;
    case 13:
        year += value;
      break;
    case 14:
      if(hour == 0 && value == -1)
      {
        hour = 23;
      }
      else if(hour == 23 && value == 1)
      {
        hour = 0;
      }
      else if(hour >= 0 && hour <= 23)
      {
        hour += value;
      }
      break;
    case 15:
      if(minute == 0 && value == -1)
      {
        minute = 59;
      }
      else if(minute == 59 && value == 1)
      {
        minute = 0;
      }
      else if(minute >= 0 && minute <= 59)
      {
        minute += value;
      }
      break;
    case 21:
      if(dayTime[0] == 0 && value == -1)
      {
        dayTime[0] = 23;
      }
      else if(dayTime[0] == 23 && value == 1)
      {
        dayTime[0] = 0;
      }
      else if(dayTime[0] >= 0 && dayTime[0] <= 23)
      {
        dayTime[0] += value;
      }
      break;
    case 22:
      if(dayTime[1] == 0 && value == -1)
      {
        dayTime[1] = 55;
      }
      else if(dayTime[1] == 55 && value == 1)
      {
        dayTime[1] = 0;
      }
      else if(dayTime[1] >= 0 && dayTime[1] <= 55)
      {
        dayTime[1] += value*5;
      }
      break;
    case 31:
      if(nightTime[0] == 0 && value == -1)
      {
        nightTime[0] = 23;
      }
      else if(nightTime[0] == 23 && value == 1)
      {
        nightTime[0] = 0;
      }
      else if(nightTime[0] >= 0 && nightTime[0] <= 23)
      {
        nightTime[0] += value;
      }
      break;
    case 32:
      if(nightTime[1] == 0 && value == -1)
      {
        nightTime[1] = 55;
      }
      else if(nightTime[1] == 55 && value == 1)
      {
        nightTime[1] = 0;
      }
      else if(nightTime[1] >= 0 && nightTime[1] <= 55)
      {
        nightTime[1] += value*5;
      }
      break;
  }
  myRTC.setDS1302Time(0, minute, hour, 0, day, month, year);
  myRTC.updateTime();
  showSetting(selection);
}

//----------------------------------------------------------------------------//HAUPTPROGRAMM
void loop() {
  checkSettings();
  if(menuItem == 0)                       //nicht im Menü, dann IdleScreen
  {
    if(millis() - timer1 >= dauer1)            //<-Anzeigenwechsel 
    {                                           //zum Wechseln der verschiedenen Anzeigen(Werte der Sensoren)
      mainScreen(page);                 
      if(page < maxScreen)
      {
        page++;
      }
      else
      {
        page = 1;
      }
      timer1=millis();
    }                                           //Anzeigenwechsel->
  }

  buttonPressed = getButton(1);               //<-Tasterabfrage // 1 für druck 0 für release
  if(buttonPressed != -1){                                                                //Hauptmenü
    lastAction = millis();
    if(settings == 0){
      if(getButtonVal(buttonPressed) == "rgt" && menuItem <= maxMenuItems){
        menuItem++;
        showMenu(menuItem);
      }
      if(getButtonVal(buttonPressed) == "lft" && menuItem > 0){
        menuItem--;
        showMenu(menuItem);
      }
      if(getButtonVal(buttonPressed) == "ok" && menuItem > 0 && menuItem < 4){
        settings = 1;
        submenu = 1;
        showSetting(menuItem*10+submenu);
      }
      else if(getButtonVal(buttonPressed) == "up" && menuItem > 3 && menuItem <= maxMenuItems){
        changeValue(menuItem, 0.5);
      }
      else if(getButtonVal(buttonPressed) == "dwn" && menuItem > 3 && menuItem <= maxMenuItems){
        changeValue(menuItem, -0.5);
      }
      if(getButtonVal(buttonPressed) == "ok" && menuItem == maxMenuItems+1){
        menuItem = 0;
      }
    }

    if(settings == 1){                                                                        //Steuerung der Untermenüs
      if(getButtonVal(buttonPressed) == "ok" && submenu == maxSubmenu[menuItem-1]+1){
        settings = 0;
        menuItem = 0;
        submenu = 0;
      }
      if(getButtonVal(buttonPressed) == "rgt" && submenu <= maxSubmenu[menuItem-1]){
        submenu++;
        showSetting(menuItem*10+submenu);
      }
      if(getButtonVal(buttonPressed) == "rgt" && submenu == maxSubmenu[menuItem-1]+1){
        showSetting(1);
      }
      if(getButtonVal(buttonPressed) == "lft" && submenu > 0){
        submenu--;
        showSetting(menuItem*10+submenu);
      }
      if(getButtonVal(buttonPressed) == "up"){
        changeSetting(menuItem*10+submenu, 1);
      }
      else if(getButtonVal(buttonPressed) == "dwn"){
        changeSetting(menuItem*10+submenu, -1);
      }
    }
  } //Tasterabfrage->
  if (millis() - lastAction > 10000){ // Wenn 10 Sekunden kein Taster gedrückt wurde, wird der mainScreen angezeigt
    menuItem = 0;
    settings = 0;
    submenu=0;
  }
}
