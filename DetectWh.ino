#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define BOTtoken "<token>"  // your Bot Token (Get from Botfather)
#define CHAT_ID "<chat-id>"
#define WH_FILE "./wh_consumed.txt"

float old = 0;
int whCount = 0;
const char* ssid = "<wifi-name>";
const char* password = "<password>";

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

const long utcOffsetInSeconds = 3600;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
int currentDay;
int currentHour;


IPAddress local_IP(192, 168, 1, 199);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); // this is optional
IPAddress secondaryDNS(8, 8, 4, 4); // this is optional

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  Serial.println("Trying to connect"); 
  // Connect to Wi-Fi
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi connected");  
  
  timeClient.begin();
  timeClient.update();
  currentDay = timeClient.getDay();
  currentHour = timeClient.getHours();

  bool success = SPIFFS.begin();

  if (success) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return;
  }
  SPIFFS.remove(WH_FILE);
  bot.sendMessage(CHAT_ID, "Bot started up", "");
  addComsumption(currentHour, 19);
  addComsumption(19, 20);
  String allConsumption = getDayConsumption();
  sendReport(allConsumption);
}


void loop() { 
  if (led_blinked() == 1){
    whCount++;    
  }
  timeClient.update();
  int newHour = timeClient.getHours();
  int newDay = timeClient.getDay();
  if (newDay != currentDay){
    String allConsumption = getDayConsumption();
    sendReport(allConsumption);
    SPIFFS.remove(WH_FILE);
    currentDay = newDay;
  }
  if (newHour != currentHour){
    addComsumption(currentHour, newHour); 
    currentHour = newHour;
    whCount = 0;
  }
  delay(10);
}

int led_blinked(){
  int sensorValue = analogRead(A0);   // read the input on analog pin 0
  if (sensorValue > old+50){
    old = sensorValue;
    return 1;
  }
  return 0;	
}

void addComsumption(int currentHour, int newHour){
  Serial.println("Adding consumption");
  File f = SPIFFS.open(WH_FILE, "a");
  char buffer[40];
  sprintf(buffer, "%d:00-%d:00 %dWh", currentHour, newHour, whCount);
  f.println(buffer);
  f.close();
}

String getDayConsumption(){
  Serial.println("Getting consumption");
  File f = SPIFFS.open(WH_FILE, "r"); 
  String content = "";
  if (!f) {
      Serial.println("file open failed");
  }  
  while(f.available()) {
    String line = f.readStringUntil('\n');
    content=content+"- "+line+"\n";
  }
  f.close();    
  return content;
}

void sendReport(String allConsumption){
  timeClient.update();  
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int currentYear = ptm->tm_year+1900;
  int currentMonth = ptm->tm_mon+1;
  int monthDay = ptm->tm_mday;
  String currentDate = String(monthDay)+"/"+String(currentMonth)+"/"+String(currentYear);  
  String report = "⚡⚡ Report for today ⚡⚡\n";
  report += allConsumption;
  bot.sendMessage(CHAT_ID, report, "markdown");

}

