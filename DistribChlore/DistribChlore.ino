//
//
//    Distributeur de Chlore
//
//  Moteur CC sur PIN D5(Enable),D7 (IN1) ,D8 (IN2)
//  SDA : A4
//  SCL : A5
//  Ecran LCD I2C
//  RTC ds3231 I2C
//  DS18B20 Temperature Piscine 
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


//Controle du moteur
#define moteurENB 5
#define moteurIN1 7
#define moteurIN2 8


// LCD definition
LiquidCrystal_I2C lcd(0x27, 16, 2);



/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)
/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)

// Configuration sauvegardee en EEPROM
struct config_t
{
  long waitingTime;
  int quantite;
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
int HeureDistrib;
char message1[16] = "";  //pour stoker message ligne 1 du LCD
char message2[16] = "";  //pour stoker message ligne 2 du LCD


unsigned long  currentTime;
unsigned long  StartTime = 0;
unsigned long  StartTimeManual = 0;

const int PinA = 3;  // Used for generating interrupts using CLK signal
const int PinB = 4;  // Used for reading DT signal
const int PinSW = 10;  // Used for the push button switch
// Keep track of last rotary value
int lastCount = 50;

// Updated by the ISR (Interrupt Service Routine)
volatile int virtualPosition = 50; //non utilisé ici

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
  lcd.print(" Starting...");
  
  Serial.println("Starting");
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

  Serial.println("Debug 1");
    
  //Initier tableau des jour pour activer Chlore
  //utiliser le num du jour 0 Dimanche
  //7 slot, 1 par jour possible, si pas configuré, mise à 9
  for(int compteur = 0 ; compteur < 7 ; compteur++)
  {
      DayDistrib[compteur]=9;
  }
   DayDistrib[0]=3; //Initié un jour pour distribution
   HeureDistrib=0; //Heure de distribution, par exemple 20h
   previous_dayWeek=9; // JOur fictif pour permettre un premier check
   FlagStart="N";
   Serial.println("Debug 2");

}


void loop() {

    DateTime now = rtc.now();

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
      lcd.print("  ");
      lcd.setCursor(0, 1);
      lcd.print (now.hour()/10,DEC);
      lcd.print (now.hour()%10,DEC);
      lcd.print(":");
      lcd.print(now.minute()/10, DEC);
      lcd.print(now.minute()%10, DEC);
      lcd.print(":");
      lcd.print(now.second()/10, DEC);
      lcd.print(now.second()%10, DEC);
      lcd.print("  ");
      lcd.print(virtualPosition); 
      
    }
//    Serial.print("Day  :");
//    Serial.println(now.dayOfTheWeek()); 
//    Serial.print("previous day :");
//    Serial.println(previous_dayWeek);
// 
    //Parcour tableau des jours configuréspour execution si on est sur un nouveau jour

    if ( previous_dayWeek != now.dayOfTheWeek() ) {    
      for(int compteur = 0 ; compteur < 7 ; compteur++)   //parcourir le tableau de jour configuré pour distribution 
      {
        if ( (now.dayOfTheWeek() == DayDistrib[0]) && (HeureDistrib == now.hour() ) && (now.dayOfTheWeek() !=  previous_dayWeek ) ){
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
        Serial.print("Start Moteur ");
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
    
    
    //Si on appuis sur le bouton, 
    //clean LCD, star compteur, moteur
    //Si plus de pression, stop compteur, affichier le compteur
    
    if ((!digitalRead(PinSW))) {
      //virtualPosition = 50;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print ("Manual action");
      StartTimeManual=millis(); // prise mesure start
      lcd.setCursor(0, 1);
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
    }

    
//FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP FIN LOOP    
}
