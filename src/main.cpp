//If Something doesn't work anymore, check cert


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "NTPtimeESP.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>

#define echoPin 4 // to pin Echo of HC-SR04
#define trigPin 0 // to pin Trig of HC-SR04
#define led 2
#define voltRead 

//Telegram Stuff
bool settings_menu = false;
bool auto_menu = false;

//SD card stuff
const char *conf_file = "CessPoolConf.txt";
const char *user_conf_file = "userConf.txt";
const int chipSelect = 4;
DynamicJsonDocument config_doc(512);

JsonObject configJson;

File configFileSDcard;


const unsigned long BOT_MTBS = 800;
unsigned long bot_lasttime;


long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement
int warning1 = 0;
int last_time_measured = 0;
int ledStatus = 0;

int warn_level = 0;
int top_distance = 0;
int diameter = 0;
int status_interval = 0;
String status_period = "";


NTPtime NTPch("pool.ntp.org");   // Choose server pool as required

strDateTime dateTime;
String aikaleima;

String BOT_TOKEN ="";
String MAIN_CHAT_ID = "";

String ssid  = "";
String password = "";


X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

JsonObject getJSonFromFile(DynamicJsonDocument *doc, String filename, bool forceCleanONJsonError = true ) {
  // open the file for reading:
  configFileSDcard = SD.open(filename);
  if (configFileSDcard) {

    DeserializationError error = deserializeJson(*doc, configFileSDcard);
    if (error) {
      // if the file didn't open, print an error:
      Serial.print(F("Error parsing JSON "));
      Serial.println(error.c_str());

      if (forceCleanONJsonError){
          return doc->to<JsonObject>();
      }
    }
 
    // close the file:
    configFileSDcard.close();
    return doc->as<JsonObject>();
  }
  else {
        // if the file didn't open, print an error:
        Serial.print(F("Error opening (or file not exists) "));
        Serial.println(filename);
 
        Serial.println(F("Empty json created"));
        return doc->to<JsonObject>();
  } 
}

int get_distance() {
  int calculated_distance = 0;
  int avarage = 0;
  for (size_t i = 0; i < 30; i++)
  {
     // Clears the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  calculated_distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
  avarage = avarage + calculated_distance;
  }
  calculated_distance = avarage/30;
  return calculated_distance;
}

String checkTime(){
  int x = 0;
  strDateTime tempDateTime;

  while (!tempDateTime.valid && x < 50){
    tempDateTime = NTPch.getNTPtime(2.0, 1);
    delay(500);
		++x;
  }
  if (tempDateTime.valid){
    dateTime = tempDateTime;
    aikaleima = aikaleima = String(dateTime.day) + "." + String(dateTime.month) + "." + String(dateTime.year) +  " / "+
                String(dateTime.hour) + ":" + String(dateTime.minute) + ":" + String(dateTime.second);
  }
  else
  {
    aikaleima = "aikaleimaa ei saatu";
    Serial.println("aikaleimaa ei saatu");
  }
  
   return aikaleima;
}

String cesspoolStatus(){

  int tempDist = get_distance();
  String currTime = checkTime();
  String status = "Yhteys laitteeseen saatu: " + currTime + ".\n";
  status += "\n";
  status += "Kaivon syvyys: " + String(tempDist) + "cm.\n";
  status += "warn_level: " + String(warn_level) + "cm.\n";
  status += "status_period: " +  status_period + ".\n";
  status += "status_interval: " +  String(status_interval) + "cm.\n";
  status += "top_distance: " +  String(top_distance) + "cm.\n";
  status += "diameter: " +  String(diameter) + ".\n";

  return status;
}

void handleNewMessages(int numNewMessages)
{
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;

    Serial.println(chat_id);

    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/mittaus"){
      distance = get_distance();
      
      bot.sendMessage(chat_id, "Pinnan etäisyys kannesta: " + String(distance) + "cm", "");
    }

    if (text == "/settings"){
     String settings = "Settings -valikko, ei toimi, ellei joku tätä päivitä joskus. SD kortilla olevan tiedoston avulla voi hoitaa päivityksen:\n";
      settings += "Alla olevilla komennoilla voit antaa käskyjä botille.\n\n";
      settings += "/warn_level : Voit vaihtaa hälytyskorkeuden lähettämällä numeron senttimetreinä.\n";
      settings += "/automation_settings : Automaattisen mittauksen asetus.\n";
      settings += "/ssid : Syötä Wifin nimi.\n";
      settings += "/password : Syötä Wifin salasana.\n";
      settings += "/cancel : Peruuta valinnat\n";
      settings += "/save : Tallenna valinnat\n";
      
      bot.sendMessage(chat_id, settings, "Markdown");
      settings_menu = true;
      unsigned long settingsTime = millis();
    }

    if (text == "/restart"){
      ESP.restart();
    }

    if (text == "/status"){ 
      bot.sendMessage(chat_id, cesspoolStatus(), ""); 
    }

    if (text == "/start"){
      String welcome = "Hei, tässä ohjeet septitankkibottia varten, " + from_name + ".\n";
      welcome += "Alla olevilla komennoilla voit antaa käskyjä botille.\n\n";
      welcome += "/status : kertoo tietoja laitteesta\n";
      welcome += "/mittaus : Mittaa pinnan etäisyyden\n";
      welcome += "/restart : Antaa resetointikäskyn\n";
      
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void readVariables(){
  BOT_TOKEN = String(configJson["bot_token"]);
  MAIN_CHAT_ID = String(configJson["main_chat_id"]);
  ssid = String(configJson["ssid"]);
  password = String(configJson["password"]);
  warn_level = configJson["warn_level"];
  top_distance = configJson["top_distance"];
  diameter = configJson["diameter"];
  status_interval = configJson["status_interval"];
  status_period = String(configJson["status_period"]);
}

void timedMeasure(){
  checkTime();
  int every_switch = 0;
  int counter = 0;
  if (status_period.equals("Y")){every_switch = 1;}
  if (status_period.equals("M")){every_switch = 2;}
  if (status_period.equals("D")){every_switch = 3;}
  if (status_period.equals("H")){every_switch = 4;}
  if (status_period.equals("MIN")){every_switch = 5;}
  if (status_period.equals("S")){every_switch = 6;}
  
  switch (every_switch)
  {    
    case 1:
      if (last_time_measured != dateTime.year){
        if (dateTime.year >=  status_interval +last_time_measured){ 
          if (dateTime.hour == 12){
            get_distance();
            Serial.println("Timed measure taken. Measured distance: " + String(distance) + "cm");
            bot.sendMessage(MAIN_CHAT_ID, "Ajastettu mittaus. Pinnan etäisyys: "
                                        + String(distance) + "cm. Tarkastus tehdään " +String(status_interval) +" vuoden välein", "");
            last_time_measured =  dateTime.year;
          }
        }
      }
      break;
    case 2:
      if (last_time_measured != dateTime.month){
        if (status_interval +last_time_measured >12){counter = (status_interval +last_time_measured) % 12;}
        else{ counter = status_interval +last_time_measured;}
        
        if (dateTime.month >=  counter){
          if (dateTime.hour == 12){
          get_distance();
          Serial.println("Timed measure taken. Measured distance: " + String(distance) + "cm");
          bot.sendMessage(MAIN_CHAT_ID, "Ajastettu mittaus. Pinnan etäisyys: "
                                        + String(distance) + "cm. Tarkastus tehdään " +String(status_interval) +" kuukauden välein.", "");
          last_time_measured =  dateTime.month;
          }
        }
      }
      break;
    case 3:
        if (last_time_measured != dateTime.day){
          if (status_interval +last_time_measured >28){counter = (status_interval +last_time_measured) % 28;}
          else{ counter = status_interval +last_time_measured;}
          
          if (dateTime.day >=  counter){
            if (dateTime.hour == 12){
            get_distance();
            Serial.println("Timed measure taken. Measured distance: " + String(distance) + "cm");
            bot.sendMessage(MAIN_CHAT_ID, "Ajastettu mittaus. Pinnan etäisyys: "
                                          + String(distance) + "cm. Tarkastus tehdään " +String(status_interval) +" päivän välein.", "");
            last_time_measured =  dateTime.day;
          }
        }
      }
      break;
  case 4:
    if (last_time_measured != dateTime.hour){
      if (status_interval +last_time_measured >24){counter = (status_interval +last_time_measured) % 24;}
      else{ counter = status_interval +last_time_measured;}

      if (dateTime.hour >= counter){   
        get_distance();
        Serial.println("Timed measure taken. Measured distance: " + String(distance) + "cm");
        bot.sendMessage(MAIN_CHAT_ID, "Ajastettu mittaus. Pinnan etäisyys: "
                                      + String(distance) + "cm. Tarkastus tehdään " +String(status_interval) +" tunnin välein.", "");
        last_time_measured =  dateTime.hour;     
      }
    }  
      break;
  case 5:
    if (last_time_measured != dateTime.minute){
      if (status_interval +last_time_measured >60){counter = (status_interval +last_time_measured) % 60;}
      else{ counter = status_interval +last_time_measured;}

      if (dateTime.minute >=  counter){   
        get_distance();
        Serial.println("Timed measure taken. Measured distance: " + String(distance) + "cm");
        bot.sendMessage(MAIN_CHAT_ID, "Ajastettu mittaus. Pinnan etäisyys: "
                                      + String(distance) + "cm. Tarkastus tehdään " +String(status_interval) +" minuutin välein.", "");
        last_time_measured =  dateTime.minute;     
      }
    }     
      break;
  case 6:
    if (last_time_measured != dateTime.second){
      if (status_interval +last_time_measured >50){counter = (status_interval +last_time_measured) % 60;}
      else{ counter = status_interval +last_time_measured;}
      Serial.println("Counter: " + String(counter) +", Second: " + String(dateTime.second) + ", Status interval: " + String(status_interval));
      
      if (dateTime.second >=  counter){   
        get_distance();
        Serial.println("Timed measure taken. Measured distance: " + String(distance) + "cm");
        bot.sendMessage(MAIN_CHAT_ID, "Ajastettu mittaus. Pinnan etäisyys: "
                                      + String(distance) + "cm. Tarkastus tehdään " +String(status_interval) +" sekunnin välein.", "");
        last_time_measured =  dateTime.second;     
      }
    }
      break;
    default:
      break;
  }
}

bool saveJSonToAFile(DynamicJsonDocument *doc, String user_conf_file) {
    SD.remove(user_conf_file);
 
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    Serial.println(F("Open file in write mode"));
    configFileSDcard = SD.open(user_conf_file, FILE_WRITE);
    if (configFileSDcard) {
        Serial.print(F("Filename --> "));
        Serial.println(user_conf_file);
 
        Serial.print(F("Start write..."));
 
        serializeJson(*doc, configFileSDcard);
 
        Serial.print(F("..."));
        // close the file:
        configFileSDcard.close();
        Serial.println(F("done."));
 
        return true;
    } else {
        // if the file didn't open, print an error:
        Serial.print(F("Error opening "));
        Serial.println(user_conf_file);
 
        return false;
    }
}

void check_warnlevel(){
      if (distance < configJson["warn_level"]){
        if( abs(warning1 -distance) >= 3 && distance > 2){
          bot.sendMessage(MAIN_CHAT_ID, "Pinnan etäisyys: " + String(distance) + "cm, on aika tilata tyhjennys", "");
          warning1 = distance;
        }
        digitalWrite(led, HIGH);
      }

      else{
        digitalWrite(led, LOW);
      }
    }

void setup()
{
  pinMode(led, OUTPUT);
  
  Serial.begin(115200);
  while (!Serial)
        continue;
    delay(500);
 
    // Initialize SD library
    while (!SD.begin(chipSelect)) {
        Serial.println(F("Failed to initialize SD library"));
        delay(1000);
    }
  Serial.println("");

  //haetaan data jsonista
  configJson = getJSonFromFile(&config_doc, user_conf_file);
  if (configJson.isNull()){
    configJson = getJSonFromFile(&config_doc, conf_file);
    Serial.println("Using default conf file.");
  }
  else
  {
    Serial.println("Found user Conf file.");
  }
  
  readVariables();
  
  configTime(2, 1, "pool.ntp.org");

  Serial.println("##  Connecting to Internet  ##");
  WiFi.mode(WIFI_STA);

  bot.updateToken(BOT_TOKEN);

  WiFi.begin(ssid, password);
  // Telegram stuff
  secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT


  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(MAIN_CHAT_ID, cesspoolStatus(), "");
}

void loop(){
  while (WiFi.status() == WL_CONNECTED){

    if (millis() - bot_lasttime > BOT_MTBS){
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

      while (numNewMessages){
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }

      bot_lasttime = millis();
    }
  
    distance = get_distance();
    Serial.println("Continous measuring. Distance: " + String(distance) + "cm");
  
    timedMeasure();

    check_warnlevel();

    delay(200);

    // while (settings_menu){
    //   if( millis(); >= settingsTime + 100000){
    //     settings_menu = false
    //     Serial.println("Settings menu timed out");
    //   }

    // }
    
  
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(led, HIGH);
    Serial.print(".");
    
  }


}
/*
/
/
/
/
/
/
/
*/