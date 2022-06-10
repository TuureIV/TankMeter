#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "NTPtimeESP.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#define echoPin 4 // to pin Echo of HC-SR04
#define trigPin 0 // to pin Trig of HC-SR04
#define led 14

//Telegram Stuff
#define BOT_TOKEN ""
#define MAIN_CHAT_ID  ""

const char * ssid = "";
const char * password = "";

const unsigned long BOT_MTBS = 1000;
unsigned long bot_lasttime;


long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement

int ledStatus = 0;

NTPtime NTPch("pool.ntp.org");   // Choose server pool as required

strDateTime dateTime;
strDateTime HWtime;
String aikaleima;


X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);


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

String checkTime()
{    
  dateTime = NTPch.getNTPtime(2.0, 1);
  if(dateTime.valid){
		aikaleima = String(dateTime.day) + "." + String(dateTime.month) + "."  + "." + String(dateTime.year) +  " / "+
                String(dateTime.hour) + ":" + String(dateTime.minute) + ":" + String(dateTime.second);
  }
  else
  {
    aikaleima = "aikaleimaa ei saatu";
  }
  
   return aikaleima;
}

String cesspoolStatus(){

  distance = get_distance();
  aikaleima = checkTime();
  String status = "Yhteys laitteeseen saatu: " + aikaleima + ".\n";
  status += "\n";
  status += "Kaivon syvyys: " + String(distance) + "\n";
  status += "Yhdistetty nettiin\n";
  status += "Vielä kai jotain\n";

  return status;
}



void setup()
{
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  delay(800);
  Serial.println("");
  Serial.println("##  Connecting to Internet  ##");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Telegram stuff
  secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org


  pinMode(trigPin, OUTPUT); // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT); // Sets the echoPin as an INPUT
   // // Serial Communication is starting with 9600 of baudrate speed

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Retrieving time: ");
  configTime(2, 1, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  bot.sendMessage(MAIN_CHAT_ID, "Bot started up", "");
    
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






// void UpdateHWTime()
// {    
//     pinMode(Led, HIGH);
//     HWtime = dateTime;
//     Serial.println(HWtime);
//     Serial.println("HW time");
//     delay(3000);
   
// }

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
  

  if (distance < 14){

    digitalWrite(led, HIGH);
    bot.sendMessage(MAIN_CHAT_ID, "Pinnan etäisyys kannesta: " + String(distance) + "cm, on aika tilata tyhjennys", "");
  }

  else
  {
    digitalWrite(led, LOW);
  }

  delay(1500);
  

  // dateTime = NTPch.getNTPtime(2.0, 1);
  // if(dateTime.valid){
  //   NTPch.printDateTime(dateTime);
  // }

  
}