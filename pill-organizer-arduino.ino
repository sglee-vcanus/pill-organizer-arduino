#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "DHT.h"

#ifndef APSSID
#define APSSID  "VCANUS_PILLO_AP"
#define APPSK   "vPillo&1"
#endif

WebServer server(80);

#define EEPROM_SIZE 20
int addr = 0;
String ip[4];

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

void readEEPROM();
void setModeAP();
void setWebServer();
void root();
void notFound();
void displayJson();

void setup() {
  Serial.begin(115200);
  Serial.println();

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
  WiFi.disconnect();
  delay(1000);
     
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
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  String result = "";
  DynamicJsonDocument doc(1024);
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  serializeJson(doc, result);
  Serial.println(result);
  server.send(200, "application/json", result);
}
