/* ESP32, ArduinoJson version 6 */
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#ifndef STASSID
#define STASSID                   ""
#define STAPSK                    ""
#endif

#ifndef APSSID
#define APSSID                    "VCANUS_PILLO_AP"
#define APPSK                     ""
#endif

#define GPIO5                     5
#define WIFI_CONNECTION_TIMEOUT   5000
String ssid = STASSID;
String passwd = STAPSK;
const char *host = "http://vcanus.com/pillo";
const uint16_t port = 80;
//WiFiMode _currentMode = WIFI_OFF;//for ESP8266
WiFiMode_t _currentMode = WIFI_OFF;//for ESP32

////WiFiClient client;
//ESP8266WebServer webServer(80);//for ESP8266
WebServer webServer(80);//for ESP32

#define EEPROM_SIZE 20
int addr = 0;
String ipAddress[4];

/**
 * Read IP address from EEPROM
 */
void readEEPROM() {
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

  //ipAddress = EEPROM.readString(addr);
  //addr += ipAddress.length() + 1;
  for (int i = 0; i < 4; i++)
  {
    ipAddress[i] = EEPROM.readString(addr);
    addr += ipAddress[i].length() + 1;
  }
}

/**
 * Internal function to set mode off
 */
void _setModeOFF() {
  WiFi.softAPdisconnect();
  WiFi.disconnect();
}

/**
 * Internal function to set mode to be AP
 */
void _setModeAP() {    
  Serial.println("Configuring access point...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(APSSID,APPSK);
  Serial.println("Wait 100ms for AP_START...");
  delay(100);

  Serial.println("Set softAPConfig");
  IPAddress Ip(ipAddress[0].toInt(), ipAddress[1].toInt(), ipAddress[2].toInt(), ipAddress[3].toInt());
  //IPAddress Ip(192, 168, 10, 1);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP mode, IP address: ");
  Serial.println(ip);
}

/**
 * Internal common function to set mode to be STA
 */
void _setModeCommonSTA() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  const char *ssidArray = ssid.c_str();
  const char *passwdArray = passwd.c_str();
  /**
   * Delete the below, later
   */
  Serial.println(ssidArray);
  Serial.println(passwdArray);
  WiFi.begin(ssidArray, passwdArray);
  int elapsedTime = 0;
  int oneDelay = 500;
  while(WiFi.status() != WL_CONNECTED) {
    delay(oneDelay);
    elapsedTime += oneDelay;
    Serial.print(".");
    if(elapsedTime > WIFI_CONNECTION_TIMEOUT) {
      Serial.print("WiFi connecion failed");
      if(_currentMode != WIFI_AP) {
        WiFi.softAPdisconnect();
        WiFi.disconnect();
        _setModeAP();
      }
      return;
    }
  }
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.println("STA mode, IP address: ");
  Serial.println(WiFi.localIP());
}

void _setModeSTA() {
  WiFi.mode(WIFI_STA);
  _setModeCommonSTA();
}

void _setModeAPSTA() {
  WiFi.mode(WIFI_AP_STA);
  _setModeCommonSTA();
}

// default mode is AP
/**
 * Set Mode
 * WIFI_OFF, WIFI_STA, WIFI_AP, WIFI_AP_STA
 */
void setMode(WiFiMode_t mode) {
  if(_currentMode == mode) {
    return;
  }
  switch(_currentMode) {
    case WIFI_OFF:
    break;
    case WIFI_STA:
      WiFi.softAPdisconnect();
      WiFi.disconnect();
      Serial.println("WiFi_STA disconnect()");
    break;
    case WIFI_AP:
      WiFi.softAPdisconnect();
      WiFi.disconnect();
      Serial.println("WiFi_AP disconnect()");
    break;
    case WIFI_AP_STA:
      WiFi.softAPdisconnect();
      WiFi.disconnect();
      Serial.println("WiFi_AP_STA disconnect()");
    break;
    default:
    Serial.println("undefined mode");
    break;
  }
  switch(mode) {
    case WIFI_OFF:
      _setModeOFF();
    break;
    case WIFI_STA:
      _setModeSTA();
    break;
    case WIFI_AP:
      _setModeAP();
    break;
    case WIFI_AP_STA:
      _setModeAPSTA();
    break;
    default:
    Serial.println("undefined mode");
    break;
  }
  delay(100);
}

void closeWebServer() {
   webServer.stop();
   webServer.close();
   delay(10);
}

void setWebServer() {
  webServer.on("/", restAPIRoot);
  webServer.on("/led/", restAPILed);
  webServer.on("/mode/", restAPIMode);
  webServer.begin(80);
  Serial.println("Http server started...");
}

void restAPIRoot() {
  Serial.println("root api is called");

  int nNetwork = WiFi.scanNetworks();
  const int capacity = JSON_OBJECT_SIZE(10);
  /* ArduinoJson version 5
  StaticJsonBuffer<capacity> jb;
  JsonObject &jsonObject = jb.createObject();
  JsonArray &jsonArray = jb.createArray();
   */
  // ArduinoJson version 6
  StaticJsonDocument<capacity> doc;
  JsonObject jsonObject = doc.to<JsonObject>();
  JsonArray jsonArray = doc.to<JsonArray>();
  for(int i=0; i<nNetwork; i++) {
    jsonArray.add(WiFi.SSID(i));
  }
  String arrayResult = "";
  String result = "";
  /* Serialization for ArduinoJson version 5
  jsonArray.printTo(arrayResult); 
  jsonObject.set("name",jsonArray);
  jsonObject.printTo(result);
   */
  // ArduinoJson version 6
  serializeJson(jsonArray, arrayResult);
  jsonObject["name"] = jsonArray;
  serializeJson(jsonObject, result);
  Serial.println(result);

  webServer.send(200,"application/json",result);
}

void restAPILed() {
  Serial.println("lamp api is called");
  for(int i=0; i<webServer.args(); i++) {
    String paraName = webServer.argName(i);
    String paraValue = webServer.arg(i);
    Serial.print(paraName);
    Serial.print(" : ");
    Serial.println(paraValue);
    if(paraName.equals("value")) {
      if(paraValue.equals("on")) {
        digitalWrite(GPIO5,1);
        webServer.send(200, "test/html", "Lamp is turned on");
      } else if(paraValue.equals("off")) {
        digitalWrite(GPIO5,0);
        webServer.send(200, "test/html", "Lamp is turned off");
      } else {
        Serial.println("undefined parameter value");
      }
    }
  }  
}

void restAPIMode() {
  Serial.println("mode api is called");
  String type = "";
//  String ssid = "";
//  String passwd = "";
//  Serial.println(webServer.args());
//  for(int i=0; i<webServer.args(); i++) {
//    String paraName = webServer.argName(i);
//    String paraValue = webServer.arg(i);
//    Serial.print(paraName);
//    Serial.print(" : ");
//    Serial.println(paraValue);
//    if(paraName.equals("type")) {
//      type = paraValue;
//    } else if(paraName.equals("ssid")) {
//      ssid=paraValue;
//    } else if(paraName.equals("passwd")) {
//      passwd=paraValue;
//    } else {
//      Serial.println("undefined parameter name");
//      webServer.send(400, "test/html", "undefined parameter name");
//      return;
//    }
//  }

  /* Deserialization for ArduinoJson version 5
  DynamicJsonBuffer jb;
  JsonObject &jsonObject = jb.parseObject(webServer.arg("plan"));
  if (!jsonObject.success()) {
   Serial.println("parseObject() failed");
   return;
  }
  */
  // ArduinoJson version 6
  DynamicJsonDocument doc(1024);
  JsonObject jsonObject = doc.to<JsonObject>();
  auto error = deserializeJson(doc, webServer.arg("plan"));
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
  }
  const char *type_temp = jsonObject["type"];
  const char *ssid_temp = jsonObject["ssid"];
  const char *passwd_temp = jsonObject["passwd"];

  type = type_temp;
  ssid = ssid_temp;
  passwd = passwd_temp;
  
  if(type.equals("WIFI_AP")) {
      webServer.send(200, "test/html", "Mode is changed to WIFI_AP");
      delay(100);
      closeWebServer();
      setMode(WIFI_AP);
      setWebServer();
    } else if(type.equals("WIFI_STA")) {
      webServer.send(200, "test/html", "Mode is changed to WIFI_STA");
      delay(100);
      closeWebServer();
      setMode(WIFI_STA);
      setWebServer();
    } else if(type.equals("WIFI_AP_STA")) {
      webServer.send(200, "test/html", "Mode is changed to WIFI_AP_STA");
      delay(100);
      closeWebServer();
      setMode(WIFI_AP_STA);
      setWebServer();
    } else {
      Serial.println("undefined mode type");
      webServer.send(400, "test/html", "undefined mode type");
    }
}

void scanNetwork() {  
  // Scan Network
  int nNetwork = WiFi.scanNetworks();
  Serial.println("scan done");
  if(nNetwork == 0) {
    Serial.println("no networks found");
  } else {
    Serial.println(nNetwork);
    Serial.println("networks found");
    for(int i=0; i<nNetwork; i++) {
      Serial.print(i+1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "" : "*");
      Serial.print("\r");
    }
  }
}

void setup() {
  pinMode(GPIO5, OUTPUT);
  digitalWrite(GPIO5,0);
  Serial.begin(115200);
  Serial.println();

  readEEPROM();
  scanNetwork();

  setMode(WIFI_AP);
  setWebServer();
}

//void httpResponse() {
//  // Check if a client has connected
//  WiFiClient checkClient = server.available();
//  if (!checkClient || !checkClient.connected()) {
//    return;
//  }
//
//  // Wait until the client sends some data
//  Serial.println("new client");
//
//  unsigned long timeout = millis();
//  while(checkClient.available() == 0) {
//    if(millis() - timeout > 3000) {
//      Serial.println(">>> Client Timeout !");
//      checkClient.stop();
//      delay(1);
//      return;
//    }
//    delay(1000);
//  }
////  if(checkClient.available()) {
////    digitalWrite(GPIO5,1);
////  } else {
////    digitalWrite(GPIO5,0);
////  }
//
////  // Read the first line of the request
////  String req = checkClient.readStringUntil('\r');
////  Serial.println(req);
////  // Match the request
////  int val;
////  if (req.indexOf("/gpio/0") != -1) {
////    val = 0;
////  } else if (req.indexOf("/gpio/1") != -1) {
////    val = 1;
////  } else {
////    Serial.println("invalid request");
////    checkClient.print("HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html><body>Not found</body></html>");
////    checkClient.stop();
////    delay(1);
////    return;
////  }
////  digitalWrite(GPIO5,val);
//
////  // Prepare the response
////  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
//////  s += (val) ? "high" : "low";
////  s += req;
////  s += "\r\n";
////  s += "Request is OK";
////  s += "\r\n";
////  s += "</html>\n";
////
////  // Send the response to the client
////  checkClient.print(s);
////  checkClient.stop();
////  delay(1);
////  Serial.println("Client disconnected");
//
//  // The client will actually be disconnected
//  // when the function returns and 'client' object is detroyed  
//}

void loop() {

//  if(_currentMode == WIFI_OFF) {
//    delay(1000);
//    return;
//  }
  
  webServer.handleClient();
  
//  httpResponse();

////  WiFiClient client;
//  if(!client.connected()) {
//    if(!client.connect(host,port)) {
//      Serial.println("connection failed");
//      delay(2000);
//      return;
//    }
//  } else {
////    Serial.print(client.remoteIP());
////    Serial.println("\n");
//  }

  delay(100);
  
//  Serial.println("sending data to server");
//  if(client.connected()) {
//    client.println("hello");
//  }

//  unsigned long timeout = millis();
//  while(client.available() == 0) {
//    if(millis() - timeout > 5000) {
//      Serial.println(">>> Client Timeout! ");
//      client.stop();
//      delay(60000);
//      return;
//    }
//  }
}
