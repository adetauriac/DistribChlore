//
//
//    Distributeur de Chlore
//
//  Moteur CC sur PIN D5(Enable),D7 (IN1) ,D8 (IN2)
//  SDA : A4
//  SCL : A5
//  Ecran LCD I2C
//  RTC ds3231 I2C  ==> Librairie RTClib mais modifiée pour ajouter la recupération de la temperature de la carte
//  DS18B20 Temperature Piscine sur D2
//
//
//
//
//
//

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"


//1Wire definition 
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;



//Controle du moteur
#define moteurENB 5
#define moteurIN1 7
#define moteurIN2 8


// LCD definition
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define timelight 10000  //mise en veille ecran après X seconde


/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600ULUR * 24L)
/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)
#define numberOfMinutes(_time_) ()
#define SECS_PER_DAY  (SECS_PER_HO(_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)

// Configuration sauvegardee en EEPROM
struct config_t
{
  long Duree;
  int Day[7];
  int Heure[7];
} configuration;


long correction, remainingTime, lastDistributionTime , elapsedLightTime, lastActionTime = 0L;
unsigned long lightDuration = 10L;

// Definition RTC 
RTC_DS3231 rtc;


char daysOfTheWeek[7][12] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};

//ClickEncoder *encoder;
//int16_t value = 0, last = 0;

//void timerIsr() {
//  encoder->service();
//}


int DayDistrib[7];
int HeureDistrib[7]; // 1 heure par jour de possible, mutlti time impossible
int previous_dayWeek;
const long Delais_Moteur = 5000; //en milliseconde
String FlagStart;
String Date;
String Horloge;
int Jour;
int Mois;
int Annee;
int Heure;
int Minute;
int Seconde;
bool Flagjour, Flagheure,Flagminute, Flagquantite = false ;

char message1[16] = "";  //pour stoker message ligne 1 du LCD
char message2[16] = "";  //pour stoker message ligne 2 du LCD
int delayTime2 = 350; // Delay between shifts
int i = 0;
int j = 0;
int k = 0;
int NumberMenu=6;
int posMenu = 0; //variable de position dans menu principal
int posSousMenu[2] = {0, 0}; // tableau pour stocker les positions de chaque sous-menu
float TempC,TempClock;


unsigned long  currentTime;
unsigned long  StartTime = 0;
unsigned long  StartTimeManual = 0;
unsigned long  StarTimeLight =0;

//Encoder rotatif Pin definition 
const int PinA = 3;  // Used for generating interrupts using CLK signal
const int PinB = 4;  // Used for reading DT signal
const int PinSW = 10;  // Used for the push button switch
// Keep track of last rotary value
int lastCount = 50;

// Updated by the ISR (Interrupt Service Routine)
volatile int virtualPosition = 50; 
volatile int lastPosition = 50;


// ------------------------------------------------------------------
// INTERRUPT     INTERRUPT     INTERRUPT     INTERRUPT     INTERRUPT 
// ------------------------------------------------------------------
void isr ()  {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  // If interrupts come faster than 5ms, assume it's a bounce and ignore
  if (interruptTime - lastInterruptTime > 5) {
    if (digitalRead(PinB) == LOW)
    {
      virtualPosition-- ; // Could be -5 or -10
    }
    else {
      virtualPosition++ ; // Could be +5 or +10
    }

    // Restrict value from 0 to +100
    virtualPosition = min(100, max(0, virtualPosition));

    // Keep track of when we were here last (no more than every 5ms)
    lastInterruptTime = interruptTime;
  }
}



// function to print the temperature for a device
float printTemperature(DeviceAddress deviceAddress)
{
  // method 1 - slower
  //Serial.print("Temp C: ");
  //Serial.print(sensors.getTempC(deviceAddress));
  //Serial.print(" Temp F: ");
  //Serial.print(sensors.getTempF(deviceAddress)); // Makes a second call to getTempC and then converts to Fahrenheit

  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);
  //Serial.print("Temp C: ");
  //Serial.print(tempC);
  //Serial.print(" Temp F: ");
  //Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
  return tempC;
}





void setup() {
  Serial.begin(9600);

  digitalWrite(moteurENB,LOW); //Moteur A ne tourne pas
  //Direction Moteur A
  digitalWrite(moteurIN1,LOW);
  digitalWrite(moteurIN2,HIGH);

  //Configuration de la RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  
  // initialisation de l'afficheur
  lcd.init(); // initialisation de l'afficheur
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" Distrib Chlore ");
  lcd.setCursor(0, 1);
  lcd.print("     V1.0       ");
  delay(2000);
  lcd.print("  Starting...   ");
  StarTimeLight=millis();
  Serial.println("Starting");
    // 1-wire locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
 // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
  delay(1000);
  lcd.clear();
  
  //pinMode(LCD_light, OUTPUT); 
  //encoder = new ClickEncoder(A1, A0, A2, 4);
  //encoder->setAccelerationEnabled(true);;

  //Timer1.initialize(1000);
  //Timer1.attachInterrupt(timerIsr);
 Serial.println("Debug 0");
  // Rotary pulses are INPUTs
  pinMode(PinA, INPUT_PULLUP);
  pinMode(PinB, INPUT_PULLUP);
  // Switch is floating so use the in-built PULLUP so we don't need a resistor
  pinMode(PinSW, INPUT_PULLUP);
  // Attach the routine to service the interrupts
  attachInterrupt(digitalPinToInterrupt(PinA), isr, LOW);

  //Serial.println("Debug 1");
    
  //Initier tableau des jour et heure pour activer Chlore
  //utiliser le num du jour 0 Dimanche
  //7 slot, 1 par jour possible, si pas configuré, mise à 9 pour jour et 25 pour Heure
 
  for(int compteur = 0 ; compteur < 7 ; compteur++)
  {
      DayDistrib[compteur]=9;
      HeureDistrib[compteur]=25; 
  }
   DayDistrib[0]=5; //Initié un jour pour distribution
   HeureDistrib[0]=23; //Heure de distribution, par exemple 20h
   previous_dayWeek=9; // JOur fictif pour permettre un premier check
   FlagStart="N";
   //Serial.println("Debug 2");
   DateTime now = rtc.now();

  
      

   

}


void loop() {
    //Define Date time
      DateTime now = rtc.now();
    navigation();
    affichage();
    
      if ((!digitalRead(PinSW))) {
        lcd.backlight();
        StarTimeLight=millis();
      }
      if ( millis()-StarTimeLight > timelight ){
          StarTimeLight=millis();
          lcd.noBacklight(); // turn off backlight
          posMenu=0;
      } 


      
    //Parcour tableau des jours configuréspour execution si on est sur un nouveau jour

    if ( previous_dayWeek != now.dayOfTheWeek() ) {    
      for(int compteur = 0 ; compteur < 7 ; compteur++)   //parcourir le tableau de jour configuré pour distribution 
      {
        if ( (now.dayOfTheWeek() == DayDistrib[0]) && (HeureDistrib[0] == now.hour() ) && (now.dayOfTheWeek() !=  previous_dayWeek ) ){
          Serial.print("Nous sommes un ");
          Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
          Serial.println();
          Serial.print("Lancement ");
          lcd.clear();
          FlagStart = "Y";
          compteur = 9;//pour ne plus parcourir la boucle au risque de lancer 2 fois le chlore
          previous_dayWeek=now.dayOfTheWeek();
          StartTime=millis();
        }
        else {
           Serial.print("Distribution déjà faite ");
           Serial.println();
           previous_dayWeek=now.dayOfTheWeek();
           compteur = 9;//pour ne plus parcourir la boucle au risque de lancer 2 fois le chlore
        
        }
      }

    }
    // Si flag execution actif + dans la durée d'execution, on fait tourner le moteur
  
    //Serial.print(FlagStart);
    if (FlagStart == "Y" ) {  //Pour indiquer que le moteur doit tourner en automatique
      if ( ( millis() - StartTime < Delais_Moteur) && FlagStart == "Y" ) {
        Serial.println("Start Moteur ");
        analogWrite(moteurENB,255); 
        sprintf(message1,"Distribution..."); 
        sprintf(message2,"Duree set : %2d s",Delais_Moteur/1000);
        lcd.setCursor(0, 0);
        lcd.print ("Distribution...");
        lcd.setCursor(0, 1);
        lcd.print (message2);

      }
      else  {
        Serial.print("Stop Moteur ");
        analogWrite(moteurENB,0);
        FlagStart = "N";
        lcd.clear();
      }
    }
    
    

    
//FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP    
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}




void navigation() {
   //detecter rotation  
  if ( virtualPosition !=lastPosition){
    lcd.backlight();
    StarTimeLight=millis();
    if  (virtualPosition > lastPosition ) {  //menu suivant
      posMenu=(posMenu+1 ) %NumberMenu ;
    } else if (virtualPosition < lastPosition ) { //menu precedent
        if (posMenu == 0) {posMenu=(NumberMenu);}
        posMenu=(posMenu-1) %NumberMenu;   
      }
    lastPosition=virtualPosition;
    Serial.print("Position menu : ");
    Serial.println(posMenu);
    
  }
}

void affichage(){
    DateTime now = rtc.now();
    switch (posMenu) { // en fonction du menu 1
    case 0: // Affichage Date et heures
      
      // Afficher Date et heure si pas d'autre action 
      if ( FlagStart == "N" ) {
        lcd.setCursor(0, 0);
        lcd.print (now.day()/10,DEC);
        lcd.print (now.day()%10,DEC);
        lcd.print("/");
        lcd.print (now.month()/10,DEC);
        lcd.print (now.month()%10,DEC);
        lcd.print("/");
        lcd.print (now.year(),DEC);
        lcd.print("      ");
        lcd.setCursor(0, 1);
        lcd.print (now.hour()/10,DEC);
        lcd.print (now.hour()%10,DEC);
        lcd.print(":");
        lcd.print(now.minute()/10, DEC);
        lcd.print(now.minute()%10, DEC);
        lcd.print(":");
        lcd.print(now.second()/10, DEC);
        lcd.print(now.second()%10, DEC);
        lcd.print("        ");
        }
      break;
    case 1: // Affichage temperature
        // Interroger Sonde Temperature Piscine
         sensors.requestTemperatures(); // It responds almost immediately. Let's print out the data
         TempC=printTemperature(insideThermometer); // Use a simple function to print out the data
         TempClock=rtc.getTemperature();
         
         lcd.setCursor(0, 0);
         lcd.print("Piscine : ");
         lcd.print(TempC);
         lcd.print("C");
         lcd.setCursor(0, 1);
         lcd.print("Boitier : ");
         lcd.print(TempClock);
         lcd.print("C");
      //lcd.print(virtualPosition); 
      
      break;
    case 2: // menu 2 : Test manuel 
         
          //allume ecran + reinitialise compteur ecran
          //clean LCD, star compteur, moteur
          //Si plus de pression, stop compteur, affichier le compteur
        
        lcd.setCursor(0, 0);
        lcd.print ("Manual action   ");
        lcd.setCursor(0,1);
        lcd.print ("Press to Start...");
       
        
      if ((!digitalRead(PinSW))) {
       
        StartTimeManual=millis(); // prise mesure start
        lcd.setCursor(0, 1);
        lcd.print ("                ");
        
        while (!digitalRead(PinSW)) {
          Serial.println("Presse");
          analogWrite(moteurENB,255); //Start moteur
          lcd.setCursor(0, 1);
          lcd.print (((millis()-StartTimeManual)/1000) );
          lcd.print (" s");
        }
          analogWrite(moteurENB,0); //Stop Moteur
          lcd.setCursor(0, 1);
          lcd.print ("                ");
          lcd.setCursor(0, 1);
          sprintf(message2,"Duree : %2d s",((millis()-StartTimeManual)/1000));
          lcd.print (message2);
          
          Serial.print("Durée : ");
          Serial.print((millis()-StartTimeManual)/1000);
          Serial.println("s");
          delay(4000);
          lcd.clear();
          StarTimeLight=millis();
      }
     break;
    case 3: // menu 3 test Save EEPROM
      lcd.setCursor(0, 0);
      lcd.print ("TEST EEPROM SAVE");
      lcd.setCursor(0, 1);
      lcd.print ("PRESS TO SAVE   ");  
       //Test EEPROM config and save
      if ((!digitalRead(PinSW))) { 
        Serial.println("Save EEPROM");
        for(int compteur = 0 ; compteur < 7 ; compteur++)
        {
          configuration.Day[compteur]=DayDistrib[compteur];
          configuration.Heure[compteur]=HeureDistrib[compteur];
          if (configuration.Day[compteur] != 9) {
            Serial.print("Conf num ");
            Serial.print(compteur);
            Serial.print(" : ");
            Serial.print(daysOfTheWeek[configuration.Day[compteur]]);
            Serial.print(" / ");
            Serial.print(HeureDistrib[compteur]);
            Serial.println("h");
           
            
          }
        }
        EEPROM_writeAnything(0, configuration);
      }
     break;
    case 4: // menu 4 afficher config EEPROM  
      lcd.setCursor(0, 0);
      lcd.print ("LOAD EEPROM...  ");
      lcd.setCursor(0, 1);
      lcd.print ("PRESS TO LOAD   ");
    if ((!digitalRead(PinSW))) { 
        Serial.println("Read EEPROM"); 
        EEPROM_readAnything(0, configuration);
        for(int compteur = 0 ; compteur < 7 ; compteur++)
          {
            DayDistrib[compteur]=configuration.Day[compteur];
            HeureDistrib[compteur]=configuration.Heure[compteur];
            if (configuration.Day[compteur] != 9) {
              Serial.print("EEPROM Conf num ");
              Serial.print(compteur);
              Serial.print(" : ");
              Serial.print(daysOfTheWeek[configuration.Day[compteur]]);
              Serial.print(" / ");
              Serial.print(HeureDistrib[compteur]);
              Serial.println("h");
            }
          }  
    }
      break;
    case 5: // Test selecteur jour/heure/quantité
      lcd.setCursor(0, 0);
      lcd.print ("Test Config     ");
      lcd.setCursor(0, 1);
      lcd.print ("                ");
      bool EndConfigAlarm=false;
      
      if ((!digitalRead(PinSW)) && !Flagjour) { // on active la boucle selecteur jour  
        Serial.println("Jour :");
        i=1;
        bool flag=true;
        char datelistlcd[7][16] = {
          "Dimanche",
          "Lundi   ", 
          "Mardi   ", 
          "Mercredi", 
          "Jeudi   ", 
          "Vendredi", 
          "Samedi  "};
        lcd.setCursor(0, 1);
        lcd.print(datelistlcd[i]);
        while (flag) {
            
            if  (virtualPosition > lastPosition ) {  //menu suivant
              i=(i+1 ) %7 ;
              lcd.setCursor(0, 1);
              lcd.print(datelistlcd[i]);

            } else if (virtualPosition < lastPosition ) { //menu precedent
            if (i == 0) {i=7;}
              i=(i-1) %7;   
              lcd.setCursor(0, 1);
              lcd.print(datelistlcd[i]);
            }
            lastPosition=virtualPosition;
            delay(150); //eviter rebond du poussoir avec risque de valider une mauvaise date
            Serial.println(daysOfTheWeek[i]);

            
            if ((!digitalRead(PinSW))){
            //on valide la date, sortir de la boucle
              flag=false;
            }
        
        }
        Serial.print("Votre choix : " );
        Serial.print(daysOfTheWeek[i]);
        Flagjour=true;
        
        // Config heure 
         if ((!digitalRead(PinSW)) && !Flagheure) { // on active la boucle selecteur jour  
        Serial.println("Heure :  ");
        i=00;
        bool flag=true;
        //lcd.setCursor(9, 1);
        lcd.print ("/");
        
        lcd.print (i/10,DEC);
        lcd.print (i%10,DEC);
        while (flag) {
            
            if  (virtualPosition > lastPosition ) {  //menu suivant
              i=(i+1 ) %24 ;
              lcd.setCursor(9, 1);
              lcd.print (i/10,DEC);
              lcd.print (i%10,DEC);

            } else if (virtualPosition < lastPosition ) { //menu precedent
              if (i == 0) {i=24;}
              i=(i-1) %24;   
              lcd.setCursor(9, 1);
              lcd.print (i/10,DEC);
              lcd.print (i%10,DEC);
            }
            lastPosition=virtualPosition;
            delay(150); //eviter rebond du poussoir avec risque de valider une mauvaise date
            Serial.println(i);
            if ((!digitalRead(PinSW))){
            //on valide la date, sortir de la boucle
              flag=false;
            }
        
        }
        Serial.print("Votre choix : " );
        Serial.print(i);
        Flagheure=true;

         // Config minute 
         if ((!digitalRead(PinSW)) && !Flagminute) { // on active la boucle selecteur jour  
        Serial.println("Minute :  ");
        i=00;
        bool flag=true;
        //lcd.setCursor(9, 1);
        lcd.print (":");
        
        lcd.print (i/10,DEC);
        lcd.print (i%10,DEC);
        while (flag) {
            
            if  (virtualPosition > lastPosition ) {  //menu suivant
              i=(i+1 ) %60 ;
              lcd.setCursor(12, 1);
              lcd.print (i/10,DEC);
              lcd.print (i%10,DEC);

            } else if (virtualPosition < lastPosition ) { //menu precedent
              if (i == 0) {i=60;}
              i=(i-1) %60;   
              lcd.setCursor(12, 1);
              lcd.print (i/10,DEC);
              lcd.print (i%10,DEC);
            }
            lastPosition=virtualPosition;
            delay(150); //eviter rebond du poussoir avec risque de valider une mauvaise date
            Serial.println(i);
            if ((!digitalRead(PinSW))){
            //on valide la date, sortir de la boucle
              flag=false;
            }
        
        }
        Serial.print("Votre choix : " );
        Serial.print(i);
        Flagminute=true;
        
        
      }
      break;
  }
    }
    }
}





void scrollInFromRight (int line, char str1[]) {
// Written by R. Jordan Kreindler June 2016
i = strlen(str1);
for (j = 16; j >= 0; j--) {
lcd.setCursor(0, line);
for (k = 0; k <= 15; k++) {
lcd.print(" "); // Clear line
}
lcd.setCursor(j, line);
lcd.print(str1);
delay(delayTime2);
}
}


void scrollInFromLeft (int line, char str1[]) {
// Written by R. Jordan Kreindler June 2016
i = 40 - strlen(str1);
line = line - 1;
for (j = i; j <= i + 16; j++) {
for (k = 0; k <= 15; k++) {
lcd.print(" "); // Clear line
}
lcd.setCursor(j, line);
lcd.print(str1);
delay(delayTime2);
}
}




