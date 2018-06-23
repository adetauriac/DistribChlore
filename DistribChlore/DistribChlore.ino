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
  int Duree[7];
  int Day[7];
  int Heure[7];
  int Minute[7];
  bool Active_Inactive[7];
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

// TO DO , faire un objet avec carateristique et methode
int DayDistrib[7];
int HeureDistrib[7];
int MinuteDistrib[7];
bool ActiveDistrib[7];
int QuantiteDistrib[7];


int previous_dayWeek;
const long Delais_Moteur = 5000; //en milliseconde
const long delai_clignotement = 500; //temps de clignotement LCD
String FlagStart;
String Date;
String Horloge;
int Jour;
int Mois;
int Annee;
int Heure;
int Minute;
int Seconde;
bool Flagjour, Flagheure, Flagminute, Flagquantite, exit_menu = false;

char message1[16] = "";  //pour stoker message ligne 1 du LCD
char message2[16] = "";  //pour stoker message ligne 2 du LCD
int delayTime2 = 350; // Delay between shifts

int i = 0;
int j = 0;
int k = 0;
int NumberMenu = 5;
int posMenu = 0; //variable de position dans menu principal
int posMenuAlarm = 0; //variable de position dans sous menu alarm
int NumberMenuAlarm = 9; //+1 to drive specific message EXIT

int posSousMenu[2] = {0, 0}; // tableau pour stocker les positions de chaque sous-menu
float TempC, TempClock;


unsigned long  currentTime;
unsigned long  StartTime = 0;
unsigned long  StartTimeManual = 0;
unsigned long  StarTimeLight = 0;
unsigned long lastmillis = 0;


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

  digitalWrite(moteurENB, LOW); //Moteur A ne tourne pas
  //Direction Moteur A
  digitalWrite(moteurIN1, LOW);
  digitalWrite(moteurIN2, HIGH);

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
  StarTimeLight = millis();
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


  //A mettre lors de l initialisation des alarmes lors de la lecture de EPROMM
  for (int compteur = 0 ; compteur < 7 ; compteur++)
  {
    DayDistrib[compteur] = 9;
    HeureDistrib[compteur] = 00;
    ActiveDistrib[compteur] = false;
    QuantiteDistrib[compteur] = 0;
    MinuteDistrib[compteur] = 00;
  }


  //Config temporaire, a terme a modifier via ecran pour save en EEPROM
  DayDistrib[0] = 5; //Initié un jour pour distribution
  HeureDistrib[0] = 23; //Heure de distribution, par exemple 20h
  ActiveDistrib[0] = true;
  QuantiteDistrib[0] = 3;  //Nombre de goblet (a convertire en seconde en fonction du temps necessaire moteur pour equivalence gobelet

  previous_dayWeek = 9; // JOur fictif pour permettre un premier check
  FlagStart = "N";
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
    StarTimeLight = millis();
  }
  if ( millis() - StarTimeLight > timelight ) {
    StarTimeLight = millis();
    lcd.noBacklight(); // turn off backlight
    posMenu = 0;
  }



  //Parcour tableau des jours configuréspour execution si on est sur un nouveau jour

  if ( previous_dayWeek != now.dayOfTheWeek() ) {
    for (int compteur = 0 ; compteur < 7 ; compteur++)  //parcourir le tableau de jour configuré pour distribution
    {
      if ( (now.dayOfTheWeek() == DayDistrib[compteur]) && (HeureDistrib[compteur] == now.hour() ) && ActiveDistrib[compteur] == true && (now.dayOfTheWeek() !=  previous_dayWeek ) ) {
        Serial.print("Nous sommes un ");
        Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
        Serial.println();
        Serial.print("Lancement ");
        lcd.clear();
        FlagStart = "Y";
        compteur = 9;//pour ne plus parcourir la boucle au risque de lancer 2 fois le chlore
        previous_dayWeek = now.dayOfTheWeek();
        StartTime = millis();
      }
      else {
        Serial.print("Distribution déjà faite ");
        Serial.println();
        previous_dayWeek = now.dayOfTheWeek();
        compteur = 9;//pour ne plus parcourir la boucle au risque de lancer 2 fois le chlore

      }
    }

  }
  // Si flag execution actif + dans la durée d'execution, on fait tourner le moteur

  //Serial.print(FlagStart);
  if (FlagStart == "Y" ) {  //Pour indiquer que le moteur doit tourner en automatique
    if ( ( millis() - StartTime < Delais_Moteur) && FlagStart == "Y" ) {
      Serial.println("Start Moteur ");
      analogWrite(moteurENB, 255);
      sprintf(message1, "Distribution...");
      sprintf(message2, "Duree set : %2d s", Delais_Moteur / 1000);
      lcd.setCursor(0, 0);
      lcd.print ("Distribution...");
      lcd.setCursor(0, 1);
      lcd.print (message2);

    }
    else  {
      Serial.print("Stop Moteur ");
      analogWrite(moteurENB, 0);
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
  //   Serial.print(lastPosition);
  //   Serial.print("/");
  //   Serial.println(virtualPosition);
  if ( virtualPosition != lastPosition) {
    lcd.backlight();
    StarTimeLight = millis();
    if  (virtualPosition > lastPosition ) {  //menu suivant
      posMenu = (posMenu + 1 ) % NumberMenu ;
    } else if (virtualPosition < lastPosition ) { //menu precedent
      if (posMenu == 0) {
        posMenu = (NumberMenu);
      }
      posMenu = (posMenu - 1) % NumberMenu;
    }
    lastPosition = virtualPosition;
    Serial.print("Position menu : ");
    Serial.println(posMenu);

  }
}


void navigation_menu_alarm() {
  //detecter rotation
  if ( virtualPosition != lastPosition) {
    lcd.backlight();
    StarTimeLight = millis();
    lcd.setCursor(0, 1);
    lcd.print("               ");
    if  (virtualPosition > lastPosition ) {  //menu suivant
      posMenuAlarm = (posMenuAlarm + 1 ) % NumberMenuAlarm ;
    } else if (virtualPosition < lastPosition ) { //menu precedent
      if (posMenuAlarm == 0) {
        posMenuAlarm = (NumberMenuAlarm);
      }
      posMenuAlarm = (posMenuAlarm - 1) % NumberMenuAlarm;
    }
    lastPosition = virtualPosition;
    Serial.print("Position menu Alarm: ");
    Serial.println(posMenuAlarm);

  }
}



void affichage() {
  DateTime now = rtc.now();
  switch (posMenu) { // en fonction du menu 1
    case 0: // Affichage Date et heures

      // Afficher Date et heure si pas d'autre action
      if ( FlagStart == "N" ) {
        lcd.setCursor(0, 0);
        lcd.print (now.day() / 10, DEC);
        lcd.print (now.day() % 10, DEC);
        lcd.print("/");
        lcd.print (now.month() / 10, DEC);
        lcd.print (now.month() % 10, DEC);
        lcd.print("/");
        lcd.print (now.year(), DEC);
        lcd.print("      ");
        lcd.setCursor(0, 1);
        lcd.print (now.hour() / 10, DEC);
        lcd.print (now.hour() % 10, DEC);
        lcd.print(":");
        lcd.print(now.minute() / 10, DEC);
        lcd.print(now.minute() % 10, DEC);
        lcd.print(":");
        lcd.print(now.second() / 10, DEC);
        lcd.print(now.second() % 10, DEC);
        lcd.print("        ");
      }
      break;
    case 1: // Affichage temperature
      // Interroger Sonde Temperature Piscine
      sensors.requestTemperatures(); // It responds almost immediately. Let's print out the data
      TempC = printTemperature(insideThermometer); // Use a simple function to print out the data
      TempClock = rtc.getTemperature();

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
      lcd.setCursor(0, 1);
      lcd.print ("Press to Start...");


      if ((!digitalRead(PinSW))) {

        StartTimeManual = millis(); // prise mesure start
        lcd.setCursor(0, 1);
        lcd.print ("                ");

        while (!digitalRead(PinSW)) {
          Serial.println("Presse");
          analogWrite(moteurENB, 255); //Start moteur
          lcd.setCursor(0, 1);
          lcd.print (((millis() - StartTimeManual) / 1000) );
          lcd.print (" s");
        }
        analogWrite(moteurENB, 0); //Stop Moteur
        lcd.setCursor(0, 1);
        lcd.print ("                ");
        lcd.setCursor(0, 1);
        sprintf(message2, "Duree : %2d s", ((millis() - StartTimeManual) / 1000));
        lcd.print (message2);

        Serial.print("Durée : ");
        Serial.print((millis() - StartTimeManual) / 1000);
        Serial.println("s");
        delay(4000);
        lcd.clear();
        StarTimeLight = millis();
      }
      break;
    case 3: // menu 3 test Save EEPROM
      lcd.setCursor(0, 0);
      lcd.print ("Conf Dosage...  ");
      lcd.setCursor(0, 1);
      lcd.print (">               ");
      break;
    case 4: // Test selecteur jour/heure/quantité
      lcd.setCursor(0, 0);
      lcd.print (" Alarme Set     ");
      lcd.setCursor(0, 1);
      lcd.print (">               ");
      exit_menu = false;
      if ((!digitalRead(PinSW))) { //Entrer dans le menu de lister les alarmes
        delay(150);
        while (!exit_menu) {
          navigation_menu_alarm();
          affichage_Gestion_Alarm();
        }
      }
      break;
  }
}


void affichage_Gestion_Alarm() {
  switch (posMenuAlarm) {
    case 7 : // SaveConfig EEPROM
      lcd.setCursor(0, 0);
      lcd.print ("> Save EEPROM           ");
      lcd.setCursor(0, 1);
      lcd.print ("                ");
      if ((!digitalRead(PinSW))) {
        delay(350);
        SaveConfig_EEPROM();
        lcd.setCursor(0, 1);
        lcd.print ("DONE            ");
        delay(2000);
        posMenuAlarm = 8;
      }

      break;
    case 8:
      lcd.setCursor(0, 0);
      lcd.print ("> Exit          ");
      lcd.setCursor(0, 1);
      lcd.print ("                ");
      if ((!digitalRead(PinSW))) {
        delay(350);
        exit_menu = !exit_menu;
        posMenuAlarm = 0;
      }
      break;
    default :
      affiche_alarme(posMenuAlarm);
      break;
  }
}


void affiche_alarme(int numAlarme) {
  DateTime now = rtc.now();
  bool exit_menu2 = false;
  int posSelectionUpdate = 0;
  lcd.setCursor(0, 0);
  lcd.print ("Alarme ");
  lcd.print (posMenuAlarm);
  lcd.setCursor(8, 0);
  lcd.print ("     ");
  lcd.print (Convert_Bool( ActiveDistrib[numAlarme]) );
  lcd.setCursor(0, 1);
  //Recuperer et afficher information alarme numAlarme
  if (DayDistrib[numAlarme] == 9) {
    lcd.print("No Config       ");
  }
  else {
    lcd.print(daysOfTheWeek[DayDistrib[numAlarme]]);
    lcd.setCursor(8, 1);
    lcd.print ("|");
    lcd.print (HeureDistrib[numAlarme] / 10, DEC);
    lcd.print (HeureDistrib[numAlarme] % 10, DEC);
    lcd.print (":");
    lcd.print (MinuteDistrib[numAlarme] / 10, DEC);
    lcd.print (MinuteDistrib[numAlarme] % 10, DEC);
    lcd.print (" ");
    lcd.print (QuantiteDistrib[numAlarme]);

  }

  //boucle pour update alarme / jour / heure / minute / Qte  si on clique
  if ((!digitalRead(PinSW)))  {
    delay(150);
    lastPosition = virtualPosition;
    int exit_loop = 0;
    int exit_update = 0;

    bool stat = false;
    int tmpOnOff, tmpDay, tmpHour, tmpMinute, TmpQuantite;
    char datelistlcd[7][16] = {
      "Dimanche",
      "Lundi   ",
      "Mardi   ",
      "Mercredi",
      "Jeudi   ",
      "Vendredi",
      "Samedi  "
    };
    //if alarm sans config, clean ligne 2 LCD
    if (DayDistrib[numAlarme] == 9) {
      tmpDay = now.dayOfTheWeek();
      lcd.setCursor(0, 1);
      lcd.print(datelistlcd[tmpDay]);
      lcd.print("|00:00 0");
    } else {
      tmpDay = DayDistrib[numAlarme];
    }

    while (!exit_update) {  //Parcourir la liste des attribut de l'alarme
      switch (posSelectionUpdate) {
        case 0: //Gestion du OnOff
          tmpOnOff = ActiveDistrib[numAlarme];

          while (!exit_loop) { //boucle pour parcourir les caracteristiques de l'alarme
            currentTime = millis();
            //             Serial.print("last : ");
            //             Serial.println(lastmillis);
            //             Serial.print("Millis : ");
            //             Serial.println(currentTime);
            //             Serial.print("currentTime-millis : ");
            //             Serial.print(currentTime - lastmillis);
            //             Serial.print(" : ");
            //             Serial.println(delai_clignotement);

            if (currentTime - lastmillis >= delai_clignotement ) {
              lastmillis = currentTime;
              if ( stat ) {
                stat = !stat;
                lcd.setCursor(13, 0);
                lcd.print (Convert_Bool(tmpOnOff));
                ;
              } else {
                lcd.setCursor(13, 0);
                lcd.print ("     ");
                stat = !stat;
              }
            }
            if  (virtualPosition != lastPosition ) {
              tmpOnOff = !tmpOnOff;
              lastPosition = virtualPosition;
              lcd.setCursor(13, 0);
              lcd.print (Convert_Bool(tmpOnOff));
            }

            if ((!digitalRead(PinSW))) { //Save new config alarm
              delay(250);
              ActiveDistrib[numAlarme] = tmpOnOff;
              posSelectionUpdate += 1;
              lcd.setCursor(13, 0);
              lcd.print (Convert_Bool(tmpOnOff));

              //FlagConfActivation=!FlagConfActivation;
              exit_loop = !exit_loop;
            }
          }
          break;
        case 1: //Gestion du jour

          exit_loop = 0;
          while (!exit_loop) {

            currentTime = millis();
            if (millis() - lastmillis > delai_clignotement ) {
              lastmillis = millis();
              if ( stat ) {
                stat = !stat;
                lcd.setCursor(0, 1);
                lcd.print (datelistlcd[tmpDay]);
                //Serial.println("Affiche");
              } else {
                lcd.setCursor(0, 1);
                lcd.print ("        ");
                stat = !stat;
                //Serial.println("Caché");
              }
            }
            //Lister les jours
            if  (virtualPosition > lastPosition ) {  //jour suivant
              tmpDay = (tmpDay + 1 ) % 7 ;
              lcd.setCursor(0, 1);
              lcd.print(datelistlcd[tmpDay]);

            } else if (virtualPosition < lastPosition ) { //jour precedent
              if (tmpDay == 0) {
                tmpDay = 7;
              }
              tmpDay = (tmpDay - 1) % 7;
              lcd.setCursor(0, 1);
              lcd.print(datelistlcd[tmpDay]);
            }
            lastPosition = virtualPosition;

            if ((!digitalRead(PinSW))) { //Save new config alarm
              delay(250);
              DayDistrib[numAlarme] = tmpDay;
              posSelectionUpdate += 1;
              lcd.setCursor(0, 1);
              lcd.print(datelistlcd[tmpDay]);
              //FlagConfActivation=!FlagConfActivation;
              exit_loop = !exit_loop;

            }
          }

          break;
        case 2: //Gestion de l heure
          tmpHour = HeureDistrib[numAlarme];
          exit_loop = 0;
          while (!exit_loop) {
            currentTime = millis();
            if (millis() - lastmillis > delai_clignotement ) {
              lastmillis = millis();
              if ( stat ) {
                stat = !stat;
                lcd.setCursor(9, 1);
                lcd.print (tmpHour / 10, DEC);
                lcd.print (tmpHour % 10, DEC);
                Serial.println("Affiche");
              } else {
                lcd.setCursor(9, 1);
                lcd.print ("  ");
                stat = !stat;
                Serial.println("Caché");
              }
            }
            //Lister les heures
            if (virtualPosition != lastPosition ) {
              if  (virtualPosition > lastPosition ) {  //Heure suivante
                tmpHour = (tmpHour + 1 ) % 24 ;
              } else if (virtualPosition < lastPosition ) { //Heure precedent
                if (tmpHour == 0) {
                  tmpHour = 24;
                }
                tmpHour = (tmpHour - 1) % 24;
              }
              lastPosition = virtualPosition;
              lcd.setCursor(9, 1);
              lcd.print (tmpHour / 10, DEC);
              lcd.print (tmpHour % 10, DEC);
            }

            if ((!digitalRead(PinSW))) { //Save new config alarm
              delay(250);
              HeureDistrib[numAlarme] = tmpHour;
              posSelectionUpdate += 1;
              lcd.setCursor(9, 1);
              lcd.print (HeureDistrib[numAlarme] / 10, DEC);
              lcd.print (HeureDistrib[numAlarme] % 10, DEC);
              //FlagConfActivation=!FlagConfActivation;
              exit_loop = !exit_loop;

            }
          }

          break;
        case 3: //Gestion des minutes
          tmpMinute = MinuteDistrib[numAlarme];
          exit_loop = 0;
          while (!exit_loop) {
            currentTime = millis();
            if (millis() - lastmillis > delai_clignotement ) {
              lastmillis = millis();
              if ( stat ) {
                stat = !stat;
                lcd.setCursor(12, 1);
                lcd.print (tmpMinute / 10, DEC);
                lcd.print (tmpMinute % 10, DEC);
              } else {
                lcd.setCursor(12, 1);
                lcd.print ("  ");
                stat = !stat;
              }
            }
            //Lister les Minutes
            if (virtualPosition != lastPosition ) {
              if  (virtualPosition > lastPosition ) {  //Heure suivante
                tmpMinute = (tmpMinute + 1 ) % 60 ;
              } else if (virtualPosition < lastPosition ) { //Heure precedent
                if (tmpMinute == 0) {
                  tmpMinute = 60;
                }
                tmpMinute = (tmpMinute - 1) % 60;
              }
              lastPosition = virtualPosition;
              lcd.setCursor(12, 1);
              lcd.print (tmpMinute / 10, DEC);
              lcd.print (tmpMinute % 10, DEC);
            }
            if ((!digitalRead(PinSW))) { //Save new config alarm
              delay(250);
              MinuteDistrib[numAlarme] = tmpMinute;
              posSelectionUpdate += 1;
              lcd.setCursor(12, 1);

              lcd.print (MinuteDistrib[numAlarme] / 10, DEC);
              lcd.print (MinuteDistrib[numAlarme] % 10, DEC);
              //FlagConfActivation=!FlagConfActivation;
              exit_loop = !exit_loop;
            }
          }

          break;
        case 4: //Gestion des nb gobelet
          TmpQuantite = QuantiteDistrib[numAlarme];
          exit_loop = 0;
          while (!exit_loop) {
            currentTime = millis();
            if (millis() - lastmillis > delai_clignotement ) {
              lastmillis = millis();
              if ( stat ) {
                stat = !stat;
                lcd.setCursor(15, 1);
                lcd.print (TmpQuantite);
              } else {
                lcd.setCursor(15, 1);
                lcd.print ("  ");
                stat = !stat;
              }
            }
            //Lister les nb gobelet
            if (virtualPosition != lastPosition ) {
              if  (virtualPosition > lastPosition ) {  //+ 1 gobelet
                TmpQuantite = (TmpQuantite + 1 ) % 10 ;
              } else if (virtualPosition < lastPosition ) { //Heure precedent
                if (TmpQuantite == 0) {
                  TmpQuantite = 10;
                }
                TmpQuantite = (TmpQuantite - 1) % 10;
              }
              lastPosition = virtualPosition;
              lcd.setCursor(15, 1);
              lcd.print (TmpQuantite);
            }
            if ((!digitalRead(PinSW))) { //Save new config alarm
              delay(250);
              QuantiteDistrib[numAlarme] = TmpQuantite;
              posSelectionUpdate += 1;
              lcd.setCursor(15, 1);

              lcd.print ( QuantiteDistrib[numAlarme]);
              //FlagConfActivation=!FlagConfActivation;
              exit_loop = !exit_loop;
              exit_update = !exit_update;
            }
          }

          break;
      }

    }
  }

  //
  //    while(!exit_menu2){
  //    if ((!digitalRead(PinSW))){
  //      delay(150);
  //      exit_menu=!exit_menu;
  //    }
  //   }
}


String Convert_Bool(bool val) {
  String str;
  if (val) {
    str = "ON ";
  } else {
    str = "OFF";
  }

  return str;
}

void SaveConfig_EEPROM() {
  //lcd.setCursor(0, 0);
  //lcd.print ("TEST EEPROM SAVE");
  //lcd.setCursor(0, 1);
  //lcd.print ("PRESS TO SAVE   ");
  //Test EEPROM config and save
  //if ((!digitalRead(PinSW))) {
  Serial.println("Save EEPROM");
  for (int compteur = 0 ; compteur < 7 ; compteur++)
  {
    configuration.Day[compteur] = DayDistrib[compteur];
    configuration.Heure[compteur] = HeureDistrib[compteur];
    configuration.Minute[compteur] = MinuteDistrib[compteur];
    configuration.Active_Inactive[compteur] = ActiveDistrib[compteur];
    configuration.Duree[compteur] = QuantiteDistrib[compteur];

    Serial.print("Save Conf num ");
    Serial.print(compteur);
    Serial.print(" : ");
    Serial.print(daysOfTheWeek[configuration.Day[compteur]]);
    Serial.print(" / ");
    Serial.print(HeureDistrib[compteur]);
    Serial.print(":");
    Serial.print(MinuteDistrib[compteur]);
    Serial.print(":");
    Serial.print(" / ");
    Serial.print(ActiveDistrib[compteur]);
    Serial.print(" / ");
    Serial.println(QuantiteDistrib[compteur]);
  }
  //EEPROM_writeAnything(0, configuration);

}

void LoadConfig_EEPROM() {

  Serial.println("Read EEPROM");
  EEPROM_readAnything(0, configuration);
  int compteur;
  for ( compteur = 0 ; compteur < 7 ; compteur++)
  {
    DayDistrib[compteur] = configuration.Day[compteur];
    HeureDistrib[compteur] = configuration.Heure[compteur];
    MinuteDistrib[compteur] = configuration.Minute[compteur];
    ActiveDistrib[compteur] = ActiveDistrib[compteur] ;
    QuantiteDistrib[compteur] = configuration.Duree[compteur];
  }
  Serial.print("EEPROM Conf num ");
  Serial.print(compteur);
  Serial.print(" : ");
  Serial.print(daysOfTheWeek[configuration.Day[compteur]]);
  Serial.print(" / ");
  Serial.print(HeureDistrib[compteur]);
  Serial.print("h");
  Serial.print(" / ");
  Serial.print(ActiveDistrib[compteur]);
  Serial.print(" / ");
  Serial.println(QuantiteDistrib[compteur]);

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




