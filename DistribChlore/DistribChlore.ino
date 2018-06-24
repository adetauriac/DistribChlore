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
#include "TachesDistrib.h"
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
#define timelight 20000  //mise en veille ecran après X seconde


///* Useful Constants */
//#define SECS_PER_MIN  (60UL)
//#define SECS_PER_HOUR (3600ULUR * 24L)
///* Useful Macros for getting elapsed time */
//#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)
//#define numberOfMinutes(_time_) ()
//#define SECS_PER_DAY  (SECS_PER_HO(_time_ / SECS_PER_MIN) % SECS_PER_MIN)
//#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
//#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)
//




// Configuration sauvegardee en EEPROM
struct config_t
{
  Taches TacheEEPROM[10];
  int DoseEEPROM;
  int TimeDoseEEPROM;
  
} configuration;


//long correction, remainingTime, lastDistributionTime , elapsedLightTime, lastActionTime = 0L;
//unsigned long lightDuration = 10L;

// Definition RTC
RTC_DS3231 rtc;


//char daysOfTheWeek[7][12] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};

//Passage en mode objet
int NbTaches = 10;
bool TachesDone[10];
Taches Tache[10];

int Dose = 20; //Dosage d'un gobelet/dose
int TimeDose = 6; // temps en seconde pour 1 gobelet
int tmpDose,tmpTempDose;

int previous_dayWeek;
long Delais_Moteur = 5000L; //en milliseconde, a recuperer de EEPROM ensuite

const long delai_clignotement = 500; //temps de clignotement LCD
String FlagStart;

bool exit_menu=false;

char message1[16] = "";  //pour stoker message ligne 1 du LCD
char message2[16] = "";  //pour stoker message ligne 2 du LCD


int NumberMenu = 5;
int posMenu = 0; //variable de position dans menu principal
int posMenuAlarm = 0; //variable de position dans sous menu alarm
int NumberMenuAlarm = NbTaches +2; //+2 to drive specific message EXIT and Save

//int posSousMenu[2] = {0, 0}; // tableau pour stocker les positions de chaque sous-menu
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


// Updated by the ISR (Interrupt Service Routine)
volatile int virtualPosition = 1000;
volatile int lastPosition = 1000;


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

    // Restrict value from 0 to +2000
    virtualPosition = min(2000, max(0, virtualPosition));

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

  // Rotary pulses are INPUTs
  pinMode(PinA, INPUT_PULLUP);
  pinMode(PinB, INPUT_PULLUP);
  // Switch is floating so use the in-built PULLUP so we don't need a resistor
  pinMode(PinSW, INPUT_PULLUP);
  // Attach the routine to service the interrupts
  attachInterrupt(digitalPinToInterrupt(PinA), isr, LOW);

  for (byte i = 0; i < 10; i++)
  {
    Tache[i].setup();
    TachesDone[i] = false;
  }

  previous_dayWeek = 9; // JOur fictif pour permettre un premier check
  FlagStart = "N";
  //DateTime now = rtc.now();

}


void loop() {
  //Define Date time
  DateTime now = rtc.now();
  char daysOfTheWeek[7][3] = {"Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"};

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

  //Si changement de journée, on reset le tableau des Taches réalisée
  if ( previous_dayWeek != now.dayOfTheWeek() ) {
    previous_dayWeek = now.dayOfTheWeek();
    for (byte i = 0 ; i < NbTaches ; i++) {
      TachesDone[i] = false;
    }
  }

  //Parcour tableau des jours configuréspour execution si on est sur un nouveau jour


  for (byte i = 0 ; i < NbTaches ; i++)  //parcourir le tableau de jour configuré pour distribution
  {
 
      Delais_Moteur= ( ((long)Tache[8].get_nbDose()) * ((long)TimeDose) )*1000; // TO DO fonction pour convertir nb dose avec temps pour 1 dose et passage en milliseconde (*1000)

    //Si tache "i" n'a pas encore été réalisé ET
    //  + on est le bon jour 
    //  + tache active 
    //  + date et heure correte
    //  + Tache activée 
    //  ==> on active la distribution
    if ( !TachesDone[i] && (now.dayOfTheWeek() == Tache[i].get_date()) && now.hour() == Tache[i].get_heure() && now.minute() == Tache[i].get_minute() && Tache[i].get_status() ) { 
      Serial.print("Nous sommes un ");
      Serial.println(daysOfTheWeek[now.dayOfTheWeek()]);
      Serial.print("Lancement ");
      lcd.clear();
      FlagStart = "Y";
      Delais_Moteur=Tache[i].get_nbDose()*TimeDose; // TO DO fonction pour convertir nb dose avec temps pour 1 dose
      Serial.print(Delais_Moteur);
      //On indique que le tache a été jouée pour la journée en cour
      TachesDone[i] = true;
      StartTime = millis();
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
  int exit_loop = 0;
  int exit_update = 0;
  //bool exit_menu=false;
  int stat = false;
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


    case 3: // menu 3 Configurer le dosage
      tmpDose = Dose;
      tmpTempDose = TimeDose;
      lastPosition = virtualPosition;


      lcd.setCursor(0, 0);
      lcd.print ("Conf 1 dose     ");
      lcd.setCursor(0, 1);
      //lcd.print (">               ");
      lcd.setCursor(0, 1);
      lcd.print ("1 dose = ");
      lcd.print (tmpDose / 100, DEC);
      lcd.print (tmpDose % 100, DEC);
      lcd.print ("g   ");

      if ((!digitalRead(PinSW))) {
        delay(300);
        //        lcd.setCursor(0, 1);
        //        lcd.print ("1 dose = ");
        //        lcd.print (tmpDose / 100, DEC);
        //        lcd.print (tmpDose % 100, DEC);
        //        lcd.print ("g   ");
        exit_loop = false;
        while (!exit_loop) {
          if (millis() - lastmillis > delai_clignotement ) {
            lastmillis = millis();
            if ( stat ) {
              stat = !stat;
              lcd.setCursor(9, 1);
              lcd.print (tmpDose / 100, DEC);
              lcd.print (tmpDose % 100, DEC);
            } else {
              lcd.setCursor(9, 1);
              lcd.print ("   ");
              stat = !stat;

            }
          }
          //Choix de la s=dose
          if (virtualPosition != lastPosition ) {
            if  (virtualPosition > lastPosition ) {  //Heure suivante
              tmpDose = (tmpDose + 10 ) % 1000 ;
            } else if (virtualPosition < lastPosition ) { //Heure precedent
              if (tmpDose == 0) {
                tmpDose = 1000;
              }
              tmpDose = (tmpDose - 10) % 1000;
            }
            lastPosition = virtualPosition;
            lcd.setCursor(9, 1);
            lcd.print (tmpDose / 100, DEC);
            lcd.print (tmpDose % 100, DEC);

          }
          //
          if ((!digitalRead(PinSW))) { //Save new dose
            delay(250);
            Dose = tmpDose;
            lcd.setCursor(9, 1);
            lcd.print (Dose / 100, DEC);
            lcd.print (Dose % 100, DEC);
            //FlagConfActivation=!FlagConfActivation;
            exit_loop = !exit_loop;
            delay(2000);
          }

        }

        // Gestion config nb seconde pour la dose selectionéz
        lcd.setCursor(0, 0);
        lcd.print ("Tps(s)for : ");
        lcd.print (Dose / 100, DEC);
        lcd.print (Dose % 100, DEC);
        lcd.print ("g");

        lcd.setCursor(0, 1);
        lcd.print ("Duree =  ");
        lcd.print (tmpTempDose / 10, DEC);
        lcd.print (tmpTempDose % 10, DEC);
        lcd.print ("s   ");
        exit_loop = false;
        while (!exit_loop) {
          if (millis() - lastmillis > delai_clignotement ) {
            lastmillis = millis();
            if ( stat ) {
              stat = !stat;
              lcd.setCursor(9, 1);
              lcd.print (tmpTempDose / 10, DEC);
              lcd.print (tmpTempDose % 10, DEC);
            } else {
              lcd.setCursor(9, 1);
              lcd.print ("  ");
              stat = !stat;

            }
          }
          //Choix de la temps en seconde pour la dose
          if (virtualPosition != lastPosition ) {
            if  (virtualPosition > lastPosition ) {  //Heure suivante
              tmpTempDose = (tmpTempDose + 1 ) % 60 ;
            } else if (virtualPosition < lastPosition ) { //Heure precedent
              if (tmpTempDose == 0) {
                tmpTempDose = 60;
              }
              tmpTempDose = (tmpTempDose - 1) % 60;
            }
            lastPosition = virtualPosition;
            lcd.setCursor(9, 1);
            lcd.print (tmpTempDose / 10, DEC);
            lcd.print (tmpTempDose % 10, DEC);

          }
          //
          if ((!digitalRead(PinSW))) { //Save new dose
            delay(250);
            TimeDose = tmpTempDose;
            lcd.setCursor(9, 1);
            lcd.print (TimeDose / 10, DEC);
            lcd.print (TimeDose % 10, DEC);
            //FlagConfActivation=!FlagConfActivation;
            exit_loop = !exit_loop;
          }


        }

      }



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
    case 5 : //TEMPO pour test : afficher objet Tache
      Serial.println(Tache[2].get_date());

  }
}


void affichage_Gestion_Alarm() {
  //int exit_menu =false;
  switch (posMenuAlarm) {
    case 10 : // SaveConfig EEPROM
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
    case 11:
      lcd.setCursor(0, 0);
      lcd.print ("> Exit          ");
      lcd.setCursor(0, 1);
      lcd.print ("                ");
      if ((!digitalRead(PinSW))) {
        delay(350);
        exit_menu = true;
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
  char datelistlcd[7][16] = {
      "Dimanche",
      "Lundi   ",
      "Mardi   ",
      "Mercredi",
      "Jeudi   ",
      "Vendredi",
      "Samedi  "
    };
  
  //String str =  Convert_Bool(Tache[numAlarme].get_status());
  //Serial.println (str);
  
  lcd.setCursor(0, 0);
  lcd.print ("Alarme ");
  lcd.print (posMenuAlarm);
  lcd.setCursor(8, 0);
  lcd.print ("     ");
  lcd.print (Convert_Bool(Tache[numAlarme].get_status()));
  lcd.setCursor(0, 1);
  //Recuperer et afficher information alarme numAlarme
  lcd.print(datelistlcd[Tache[numAlarme].get_date()]);
  lcd.setCursor(8, 1);
  lcd.print ("|");
  lcd.print (Tache[numAlarme].get_heure() / 10, DEC);
  lcd.print (Tache[numAlarme].get_heure() % 10, DEC);
  lcd.print (":");
  lcd.print (Tache[numAlarme].get_minute() / 10, DEC);
  lcd.print (Tache[numAlarme].get_minute() % 10, DEC);
  lcd.print (" ");
  lcd.print (Tache[numAlarme].get_nbDose());


  //boucle pour update alarme / jour / heure / minute / Qte  si on clique
  if ((!digitalRead(PinSW)))  {
    delay(150);
    lastPosition = virtualPosition;
    int exit_loop = 0;
    int exit_update = 0;

    bool stat = false;
    int tmpOnOff, tmpDay, tmpHour, tmpMinute, TmpQuantite;
    
    while (!exit_update) {  //Parcourir la liste des attribut de l'alarme
      switch (posSelectionUpdate) {
        case 0: //Gestion du OnOff
          tmpOnOff =  Tache[numAlarme].get_status();

          while (!exit_loop) { //boucle pour parcourir les caracteristiques de l'alarme
            currentTime = millis();
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
              Tache[numAlarme].set_status(tmpOnOff);
              posSelectionUpdate += 1;
              lcd.setCursor(13, 0);
              lcd.print (Convert_Bool( Tache[numAlarme].get_status()));
              exit_loop = !exit_loop;
            }
          }
          break;
        case 1: //Gestion du jour
          tmpDay = Tache[numAlarme].get_date();
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
              Tache[numAlarme].set_date(tmpDay);
              posSelectionUpdate += 1;
              lcd.setCursor(0, 1);
              lcd.print(datelistlcd[Tache[numAlarme].get_date()]);
              exit_loop = !exit_loop;
            }
          }

          break;
        case 2: //Gestion de l heure
          tmpHour = Tache[numAlarme].get_heure();
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
              Tache[numAlarme].set_heure(tmpHour);
              posSelectionUpdate += 1;
              lcd.setCursor(9, 1);
              lcd.print (Tache[numAlarme].get_heure() / 10, DEC);
              lcd.print (Tache[numAlarme].get_heure() % 10, DEC);
              //FlagConfActivation=!FlagConfActivation;
              exit_loop = !exit_loop;

            }
          }

          break;
        case 3: //Gestion des minutes
          tmpMinute = Tache[numAlarme].get_minute();
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
              Tache[numAlarme].set_minute(tmpMinute);
              posSelectionUpdate += 1;
              lcd.setCursor(12, 1);

              lcd.print (Tache[numAlarme].get_minute() / 10, DEC);
              lcd.print (Tache[numAlarme].get_minute() % 10, DEC);
              //FlagConfActivation=!FlagConfActivation;
              exit_loop = !exit_loop;
            }
          }

          break;
        case 4: //Gestion des nb gobelet
          TmpQuantite = Tache[numAlarme].get_nbDose();
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
              Tache[numAlarme].set_nbDose(TmpQuantite);
              posSelectionUpdate += 1;
              lcd.setCursor(15, 1);

              lcd.print ( Tache[numAlarme].get_nbDose());
              //FlagConfActivation=!FlagConfActivation;
              exit_loop = !exit_loop;
              exit_update = !exit_update;
            }
          }

          break;
      }

    }
  }
}


String Convert_Bool(bool val) {
  String str;
  if (val == 1) {
    str = "ON ";
  } 
  else {
    str = "OFF";
  }

  return str;
}

void SaveConfig_EEPROM() {

  Serial.println("Save EEPROM");
  for (int compteur = 0 ; compteur <= NbTaches ; compteur++)
  {
    configuration.TacheEEPROM[compteur] = Tache[compteur];
//    
//    Serial.print("Save Conf num ");
//    Serial.print(compteur);
//    Serial.print(" : ");
//    Serial.print(daysOfTheWeek[configuration.TacheEEPROM[compteur].get_date()]);
//    Serial.print(" / ");
//    Serial.print(configuration.TacheEEPROM[compteur].get_heure());
//    Serial.print(":");
//    Serial.print(configuration.TacheEEPROM[compteur].get_minute());
//    Serial.print(" / ");
//    Serial.print(configuration.TacheEEPROM[compteur].get_nbDose());
//    Serial.print(" / ");
//    Serial.println(configuration.TacheEEPROM[compteur].get_status());
  }
  configuration.DoseEEPROM=Dose;
  configuration.TimeDoseEEPROM=TimeDose;
  
  //EEPROM_writeAnything(0, configuration);

}

void LoadConfig_EEPROM() {

//  Serial.println("Read EEPROM");
//  EEPROM_readAnything(0, configuration);
//  int compteur;
//  for ( compteur = 0 ; compteur < 7 ; compteur++)
//  {
//    DayDistrib[compteur] = configuration.Day[compteur];
//    HeureDistrib[compteur] = configuration.Heure[compteur];
//    MinuteDistrib[compteur] = configuration.Minute[compteur];
//    ActiveDistrib[compteur] = ActiveDistrib[compteur] ;
//    QuantiteDistrib[compteur] = configuration.Duree[compteur];
//  }
//  Serial.print("EEPROM Conf num ");
//  Serial.print(compteur);
//  Serial.print(" : ");
//  Serial.print(daysOfTheWeek[configuration.Day[compteur]]);
//  Serial.print(" / ");
//  Serial.print(HeureDistrib[compteur]);
//  Serial.print("h");
//  Serial.print(" / ");
//  Serial.print(ActiveDistrib[compteur]);
//  Serial.print(" / ");
//  Serial.println(QuantiteDistrib[compteur]);

}






