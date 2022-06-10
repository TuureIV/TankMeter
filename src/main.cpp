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

//Telegram Stuff


// struct Config {
//   String bot_token;
//   String main_chat_id;
//   String ssid;
//   String password;
//   int warn_level;
//   int top_distance;
//   int diameter;
//   int status_every;
//   String status_period;
// };



//SD card stuff
const char *conf_file = "CessPoolConf.txt";
const char *user_conf_file = "userConf.txt";
const int chipSelect = 4;
DynamicJsonDocument config_doc(512);

JsonObject configJson;

File configFileSDcard;


const unsigned long BOT_MTBS = 1000;
unsigned long bot_lasttime;


long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement
int warning1 = 0;

int ledStatus = 0;

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
  status += "Kaivon syvyys: " + String(tempDist) + "\n";
  status += "Yhdistetty nettiin\n";
  status += "Vielä kai jotain\n";

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

    if (text == "/mittaus")
    {
      distance = get_distance();
      
      bot.sendMessage(chat_id, "Pinnan etäisyys kannesta: " + String(distance) + "cm", "");
    }

    if (text == "/ledoff")
    {
      ledStatus = 0;
      digitalWrite(led, LOW); // turn the LED off (LOW is the voltage level)
      bot.sendMessage(chat_id, "Led is OFF", "");
    }

    if (text == "/status")
    { 
      bot.sendMessage(chat_id, cesspoolStatus(), ""); 
    }

    if (text == "/start")
    {
      String welcome = "Hei, tässä ohjeet septitankkibottia varten, " + from_name + ".\n";
      welcome += "Alla olevilla komennoilla voit antaa käskyjä botille.\n\n";
      welcome += "/status : kertoo tietoja laitteesta\n";
      welcome += "/mittaus : Mittaa pinnan etäisyyden kanteen\n";
      welcome += "/reset : Antaa resetointikäskyn\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void timedMeasure(String every, int interval){


  //todo
  
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
  if (!configJson.isNull()){
    configJson = getJSonFromFile(&config_doc, conf_file);
    Serial.println("Using default conf file.");
  }

  configTime(2, 1, "pool.ntp.org");

  Serial.println("##  Connecting to Internet  ##");
  WiFi.mode(WIFI_STA);
  ssid = String(configJson["ssid"]);
  password = String(configJson["password"]);
  BOT_TOKEN = String(configJson["bot_token"]);
  MAIN_CHAT_ID = String(configJson["main_chat_id"]);

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

void loop()
{

if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
  
  distance = get_distance();
  Serial.println("Measured distance: " + String(distance) + "cm");
  

  if (distance < configJson["warn_level"]){
    
    if( abs(warning1 -distance) >= 3 && distance > 2){
     
      bot.sendMessage(MAIN_CHAT_ID, "Pinnan etäisyys kannesta: " + String(distance) + "cm, on aika tilata tyhjennys", "");
      warning1 = distance;
    }
    digitalWrite(led, HIGH);
  }

  else
  {
    digitalWrite(led, LOW);
  }

  delay(1500);
  
}