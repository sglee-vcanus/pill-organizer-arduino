#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "time.h"
#include "DHT.h"

// custom
#ifndef STASSID
#define STASSID "VC240"
#define STAPSK  "JumpUp+1"
#endif

#ifndef APSSID
#define APSSID  "VCANUS_PILLO_AP"
#define APPSK   "vPillo&1"
#endif

WebServer server(80);

#define EEPROM_SIZE 20
int addr = 0;
String ip[4];

const char* ntpServer = "pool.ntp.org";
uint8_t timeZone = 9;
uint8_t summerTime = 0;
struct tm timeinfo;
int sendHour;
int sendMin;
int sendSec;
int sendDay;
int sendMonth;
int sendYear;

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

void readEEPROM();
void setModeAP();
void setWebServer();
void root();
void notFound();
void displayJson();
void getLocalTime();
void connectToWiFi();

void setup() {
  Serial.begin(115200);
  Serial.println();

  // settings for reading local time
  connectToWiFi();
  configTime(3600 * timeZone, 3600 * summerTime, ntpServer);
  getLocalTime();
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  // settings for web server
  readEEPROM();  
  setModeAP();
  dht.begin();
  setWebServer();
}

void loop() {
  server.handleClient();
  delay(100);
}

void readEEPROM() {
  /* Read IP address from EEPROM */
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

  for (int i = 0; i < 4; i++)
  {
    ip[i] = EEPROM.readString(addr);
    addr += ip[i].length() + 1;
  }
}

void setModeAP() {
  /* Set mode to be AP*/  
  Serial.println("Configuring access point...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(APSSID,APPSK);
  Serial.println("Wait 100ms for AP_START...");
  delay(100);

  Serial.println("Set softAPConfig");
  IPAddress Ip(ip[0].toInt(), ip[1].toInt(), ip[2].toInt(), ip[3].toInt());
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP mode, SSID: "); Serial.print(APSSID);
  Serial.print(" / IP address: "); Serial.println(myIP);
}

void setWebServer() {
  server.on("/", root);
  server.on("/dhtjson", displayJson);
  server.onNotFound(notFound);
  
  server.begin();
  Serial.println("Server started");
}

void root() {
  /* Set index page */
  String content = "<h3>Connected!</h3><br>";
  content += "<a href=\"/dhtjson\">click here to check sensor data</a>";
  server.send(200, "text/html", content);
}

void notFound() {
  /* Set not found page */
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void displayJson() {
  /* Set '/dhtjson' page */
  delay(1000);
  getLocalTime();
  String measuredTime = "";
  measuredTime = String(sendYear) + "/" + String(sendMonth) + "/" + String(sendDay) + " ";
  measuredTime += String(sendHour) + ":" + String(sendMin) + ":" + String(sendSec);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  String result = "";
  DynamicJsonDocument doc(1024);
  doc["measured_time"] = measuredTime;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  serializeJson(doc, result);
  Serial.println(result);
  server.send(200, "application/json", result);
}

void getLocalTime() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  sendHour = timeinfo.tm_hour;
  sendMin = timeinfo.tm_min;
  sendSec = timeinfo.tm_sec;

  sendDay = timeinfo.tm_mday;
  sendMonth = timeinfo.tm_mon + 1;
  sendYear = timeinfo.tm_year + 1900;
}

void connectToWiFi() {
  WiFi.disconnect();
  delay(1000);
  
  WiFi.begin(STASSID, STAPSK);
  Serial.println("Connecting");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  Serial.println();
  Serial.print("Connected to SSID : ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address allotted to ESP : ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address of ESP : ");
  Serial.println(WiFi.macAddress());
  Serial.print("Signal strength is : ");
  Serial.println(WiFi.RSSI());
}
