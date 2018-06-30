/*
 
  ESP-01 pinout:

  GPIO 2 - DataIn
  GPIO 1 - LOAD/CS
  GPIO 0 - CLK

  ------------------------
  NodeMCU 1.0 pinout:

  D8 - DataIn
  D7 - LOAD/CS
  D6 - CLK
*/


#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define NUM_MAX 4
#define ROTATE 90

// for ESP-01 module
//#define DIN_PIN 2 // D4
//#define CS_PIN  3 // D9/RX
//#define CLK_PIN 0 // D3

// for NodeMCU 1.0
#define DIN_PIN 15  // D8
#define CS_PIN  13  // D7
#define CLK_PIN 12  // D6

//#include "max7219.h"
//#include "fonts.h"


String msg;
int rxPin = 5;
int txPin = 4;
int setPin = 16;
SoftwareSerial hc12(rxPin , txPin);


// =======================================================================
// Your config below!
// =======================================================================
//const char* ssid     = "FREENONO007";  // SSID of local network
const char* ssid     = "TP-LINK_302B27";
const char* password = "fffff00000nono007";             // Password on network
long utcOffset = -2;                              

String result;
char host[]="192.168.0.5";


// =======================================================================

void setup() 
{
  Serial.begin(9600);
  //INIT HC12
    pinMode(setPin,OUTPUT);
    // passage en mode commande
    digitalWrite(setPin,LOW);
    hc12.begin(9600);
    // Passage du module sur le canal 1, en 9600bps et à 20dBm
    hc12.print(F("AT+DEFAULT"));
    // Délais pour que le module traite la commande 
    delay(100);
    // passage en mode transparent
    digitalWrite(setPin,HIGH);

  
//  initMAX7219();
// sendCmdAll(CMD_SHUTDOWN,1);
//  sendCmdAll(CMD_INTENSITY,8);
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
//  printStringWithShift(" WiFi ...~",15,font,' ');
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);
  }
  Serial.println("");
  Serial.print("Connected: "); Serial.println(WiFi.localIP());
  Serial.println("Getting data ...");
  
//  printStringWithShift(" YT ...~",15,font,' ');
}
// =======================================================================

//long viewCount, viewCount24h=-1, viewsGain24h;
//long subscriberCount, subscriberCount1h=-1, subscriberCount24h=-1, subsGain1h=0, subsGain24h=0;
//long videoCount;
long HC,HP,PAPP;
const char*  DATE_releve;
int cnt = 0;
unsigned long time1h, time24h;
long localEpoc = 0;
long localMillisAtUpdate = 0;
int h, m, s;
String date;

void loop()
{
  if(Serial.available()){
    msg = Serial.readString();
    Serial.print(F("SND : "));
    Serial.println(msg);
    hc12.print(msg);
  }
  if(hc12.available()){
    Serial.print(F("RCV : "));
    Serial.print(hc12.readString());
    //Serial.println(F("°C"));
  }
 //getData();
 SendData();
}
// =======================================================================

int dualChar = 0;



// =======================================================================

int charWidth(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  return pgm_read_byte(data + 1 + ch * len);
}

// =======================================================================

const char *ytHost = "192.168.0.5";



void SendData(){
  WiFiClient client;
String url = "/~HC2/teleinfo/teleinfo_json.php?compteur=CONSO&data=direct";

if (client.connect(host, 80)) { //starts client connection, checks for connection
client.print(String("GET ") + url + " HTTP/1.1\r\n" +
"Host: " + host + "\r\n" +
"Connection: close\r\n\r\n");
Serial.println("Server is accessible");
} else {
Serial.println("connection failed"); //error message if no client connect
Serial.println();
}
delay(5000);
result = "";
while(client.available()){
    String line = client.readStringUntil('\r'); // découpe ligne par ligne
    String buf="";
    //Serial.print(line);

    Serial.println("Start decode");
    int startJson=0, dateFound=0;
    while (client.connected() && client.available()) {
      line = client.readStringUntil('\n');
      //Serial.println(line[0]);
      if(line[0]=='{') startJson=1;
      if(startJson) {
        for(int i=0;i<line.length();i++)
          if(line[i]=='[' || line[i]==']') line[i]=' ';
          buf+=line+"\n"; 
        //Serial.println("Value buffer");
        
      }
      

      
  }
//Serial.println(buf);


//int repeatCounter = 10;
//  while (!client.available() && repeatCounter--) {
//    Serial.println("y."); delay(500);
//  }

//while (client.available()) { //connected or data available
//char c = client.read(); //gets byte from ethernet buffer
//Serial.print(c);
//result = result+c;
//}


//    if(!dateFound && line.startsWith("date: ")) {
//      dateFound = 1;
//      date = line.substring(6, 22);
//      h = line.substring(23, 25).toInt();
//      m = line.substring(26, 28).toInt();
//      s = line.substring(29, 31).toInt();
//      localMillisAtUpdate = millis();
//      localEpoc = (h * 60 * 60 + m * 60 + s);
//      Serial.println(date);

  
  DynamicJsonBuffer jsonBuf;
  JsonObject &root = jsonBuf.parseObject(buf);
  if (!root.success()) {
    Serial.println("parseObject() failed");
    //printStringWithShift("json error!",30,font,' ');
    delay(10);
    //return -1;
  }
  HC = root["HC"];
  HP = root["HP"];
  DATE_releve = root["date"];
  PAPP = root ["PAPP"];
  Serial.println(DATE_releve);
  Serial.print("HC :"); Serial.println(HC);
  Serial.print("HP :"); Serial.println(HP);
  Serial.print("PAPP :");Serial.println(PAPP);
  
//  subscriberCount = root["items"]["statistics"]["subscriberCount"];
//  videoCount      = root["items"]["statistics"]["videoCount"];

  //Serial.println(result);
  client.stop(); //stop client
  //Serial.println("end of function");
  delay(10000);
}
  }


// =======================================================================
