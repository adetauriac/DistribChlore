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
//#define INPUT_SIZE 100
#define MAX_WORLD_COUNT 5
#define MIN_WORLD_COUNT 5
char *Words[MAX_WORLD_COUNT];
char *StringToParse;

char SerialByteIn;                              // Temporary variable
char HC12ByteIn;                                // Temporary variable
String HC12ReadBuffer = "";                     // Read/Write Buffer 1 for HC12
String SerialReadBuffer = "";                   // Read/Write Buffer 2 for Serial
boolean SerialEnd = false;                      // Flag to indicate End of Serial String
boolean HC12End = false;                        // Flag to indiacte End of HC12 String
boolean commandMode = false;                    // Send AT commands
char *b;
  


// =======================================================================
// Your config below!
// ======================================================================
const char* ssid     = "*************";
const char* password = "**************";             // Password on network
long utcOffset = -2;

String result;
char host[] = "192.168.0.5";
byte word_count;
String TempPiscine,TempBoitier,DateMesure;

// =======================================================================

void setup()
{
  Serial.begin(9600);
  //INIT HC12
  pinMode(setPin, OUTPUT);
  // passage en mode commande
  digitalWrite(setPin, LOW);
  hc12.begin(9600);
  // Passage du module sur le canal 1, en 9600bps et à 20dBm
  hc12.print(F("AT+DEFAULT"));
  // Délais pour que le module traite la commande
  delay(100);
  // passage en mode transparent
  digitalWrite(setPin, HIGH);
  HC12ReadBuffer.reserve(64); 


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
long HC, HP, PAPP;
const char*  DATE_releve;
int cnt = 0;
unsigned long time1h, time24h;
long localEpoc = 0;
long localMillisAtUpdate = 0;
int h, m, s;
String date;


void loop()
{
 
  if (Serial.available()) {
    msg = Serial.readString();
    Serial.print(F("SND : "));
    Serial.println(msg);
    hc12.print(msg);
  }
  while (hc12.available()) {                    // While Arduino's HC12 soft serial rx buffer has data
    HC12ByteIn = hc12.read();                   // Store each character from rx buffer in byteIn
    HC12ReadBuffer += char(HC12ByteIn);         // Write each character of byteIn to HC12ReadBuffer
    if (HC12ByteIn == '\n') {                   // At the end of the line
      HC12End = true;                           // Set HC12End flag to true
    }
  }
  
  if (HC12End ){
    Serial.println(HC12ReadBuffer);
    HC12End = false;
    char charBuf[60];
    HC12ReadBuffer.toCharArray(charBuf, 60); 
    word_count = split_message(charBuf);
    if (word_count >= MIN_WORLD_COUNT) {
      print_message(word_count);
    }
    HC12ReadBuffer = ""; 
    Serial.println(TempPiscine);
    Serial.println(TempBoitier);
    Serial.println(DateMesure);
      
    SendData(DateMesure,TempPiscine,TempBoitier);
  }
 
  //getData();
  //SendData();
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

byte split_message(char* str) {
  byte word_count = 0; //number of words
  char * item = strtok (str, "|"); //getting first word (uses space & comma as delimeter)

  while (item != NULL) {
    if (word_count >= MAX_WORLD_COUNT) {
      break;
    }
    Words[word_count] = item;
    item = strtok (NULL, "|"); //getting subsequence word
    word_count++;
  }
  return  word_count;
}

////////// ////////// ////////// ////////// ////////// ////////// //////////
// Send array over serial
////////// ////////// ////////// ////////// ////////// ////////// //////////
void print_message(byte word_count) {
  
  //if (word_count >= MIN_WORLD_COUNT) {
  //Serial.print("Word count : ");  Serial.println(word_count);
  for (byte sms_block = 0; sms_block < word_count; sms_block++) {
    Serial.print("Word "); Serial.print(sms_block + 1);  Serial.print(" : ");
    Serial.println(Words[sms_block]);
    
    switch (sms_block) {
      case 0: //dateTime
        DateMesure=String(Words[sms_block]);
        break; 
//      case 1: //ID Recepteur
//        DateMesure=Words[sms_block];
//        break; 
//      case 2: //ID Emetteur
//        DateMesure=Words[sms_block];
//        break;
      case 3: //Donnée Capteur  
         TempPiscine= String(Words[sms_block]).substring(4,9);
        
        break;   
      case 4: //Donnée Capteur
        TempBoitier= String(Words[sms_block]).substring(4,9);
        
        break;  
    }
  }
  Serial.println("--------------------");
  //}
}




void SendData(String tmpdateMesure, String tmpTempPiscine, String tmpTempBoitier) {
  WiFiClient client;
  String url = "http://192.168.0.5/~HC2/LoadData/piscine_load.php?tempPiscine="+tmpTempPiscine+"&dateHeure="+tmpdateMesure+"&TempBoitier="+tmpTempBoitier;
  url=urlencode(url);
  Serial.println(url);
  
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

}

void SendData2() {
  WiFiClient client;
  String url = "/~HC2/teleinfo/teleinfo_json.php?compteur=CONSO&data=direct";
  //String url = "http://192.168.0.5/~HC2/LoadData/piscine_load.php?tempPiscine="+TempPiscine+"&dateHeure="+dateMesure+"&TempBoitier="+TempBoitier

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
  while (client.available()) {
    String line = client.readStringUntil('\r'); // découpe ligne par ligne
    String buf = "";
    //Serial.print(line);

    Serial.println("Start decode");
    int startJson = 0, dateFound = 0;
    while (client.connected() && client.available()) {
      line = client.readStringUntil('\n');
      //Serial.println(line[0]);
      if (line[0] == '{') startJson = 1;
      if (startJson) {
        for (int i = 0; i < line.length(); i++)
          if (line[i] == '[' || line[i] == ']') line[i] = ' ';
        buf += line + "\n";
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
    Serial.print("PAPP :"); Serial.println(PAPP);

    //  subscriberCount = root["items"]["statistics"]["subscriberCount"];
    //  videoCount      = root["items"]["statistics"]["videoCount"];

    //Serial.println(result);
    client.stop(); //stop client
    //Serial.println("end of function");
    delay(10000);
  }
}

String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= "%20";
      } else {
        encodedString+=c;
      }
//      } else{
//        code1=(c & 0xf)+'0';
//        if ((c & 0xf) >9){
//            code1=(c & 0xf) - 10 + 'A';
//        }
//        c=(c>>4)&0xf;
//        code0=c+'0';
//        if (c > 9){
//            code0=c - 10 + 'A';
//        }
//        code2='\0';
//        encodedString+='%';
//        encodedString+=code0;
//        encodedString+=code1;
//        //encodedString+=code2;
//      }
      yield();
    }
    return encodedString;
    
}

unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}

// =======================================================================
