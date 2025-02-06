#include "WiFi.h"
#include "ESPAsyncWebServer.h" // Use ESP32_Async_Web_Server library  https://github.com/byHrast/byHrast_FlatPanel_DustCover/tree/main/ESP32
#include "SPIFFS.h"
#include <ESP32Servo.h>
ESP32PWM pwm;

// Defaultne WiFi postavke
const char* ssid     = "FlatPanel";
const char* password = "123456789";
// Set your Static IP address
IPAddress local_IP(192, 168, 1, 112);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
const char* hostname = "FlatPanelByHrast";

//Servo varijable
Servo myservo;  // create servo object to control a servo
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33
int servoPin = 13;
int servoENA = 32;

// Definiranje IO
const int ledPWM = 0;
const int ledENA = 26;
int ledMax = 100;
const int heaterENA = 27;

int thermistorPin1 = 25;   // Thermistor u panelu
int thermistorValue1 = 0;
int thermistorPin2 = 15;   // Thermistor na plocici
int thermistorValue2 = 0;

//Pomocne varijable
String ledState = "OFF";
String heaterState = "OFF";
String coverState = "CLOSED";
int pos = 180;  // variable to store the servo position

// Timer bez delay-a varijable
unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 1000;  // interval at which to blink (milliseconds)

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Ova fja obrađuje u html-u varijable omeđene s % i generira vrijednosti
String processor(const String& var) {
  // Return LED state
  if (var == "STATE") return ledState;

  // Determine cover state based on position
  if (var == "STATE2") return (pos >= 180) ? "CLOSED" : (pos <= 0) ? "OPEN" : "MOVING";

  // Return heater state based on heaterENA pin state
  if (var == "STATE3") return digitalRead(heaterENA) ? "ON" : "OFF";

  // Retrieve Wi-Fi settings from SPIFFS
  if (var == "SSID") return getValue(spiffsRead("WL"), ';', 0);
  if (var == "PASS") return getValue(spiffsRead("WL"), ';', 1);
  if (var == "iP") return getValue(spiffsRead("WL"), ';', 2);

  // Default return if no match
  return String();
}



void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.attach(servoPin, 500, 2400);  // attaches the servo on pin 18 to the servo object

  pinMode(servoENA, OUTPUT);
  pinMode(ledENA, OUTPUT);
  pinMode(heaterENA, OUTPUT);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    //Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  initWiFi();

  server.on("/", HTTP_GET, handleRootRequest);
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/jquery.min.js", "text/css");
  });
  server.on("/LEDstatus.htm", HTTP_GET, handleLEDStatus);
  server.on("/HEATERstatus.htm", HTTP_GET, handleHeaterStatus);
  server.on("/COVERstatus.htm", HTTP_GET, handleCoverStatus);

  server.begin();
}

void loop() {
  processSerialCommand();
}

String getValue(String data, char separator, int index) ///////// fja koja  dijeli veliki string na segmente stringova odvojene separatorom, npr., zerez ili točka zarez
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

char* string2char(String command) { //// fja koja dijeli string u char
  if (command.length() != 0) {
    char *p = const_cast<char*>(command.c_str());
    return p;
  }
}


String spiffsRead (String FileName) { /// citanje spiffs datoteka
  File file = SPIFFS.open("/" + FileName + ".txt", FILE_READ);
  String Sval;
  if (!file) {
    ////Serial.println("Failed to open file for reading");
  }
  while (file.available()) {
    Sval = file.readString(); /////////////////pročitana datoteka će biti spremljena u varijablu Sval i fja vraća tu varijablu
    //////Serial.println("Učitavam : " + FileName + " : " + Sval);
  }
  file.close();
  return Sval;
}

String spiffsWrite (String filename, String podaci) { ////fja koja zapisuje podatke u odgovarajucu txt datoteku
  File file = SPIFFS.open("/" + filename + ".txt", FILE_WRITE);

  if (!file) {
    ////Serial.println("There was an error opening the file for writing");
  }
  if (file.print(podaci)) {

  } else {
    ////Serial.println("File write failed");
  }
  file.close();
  return String();
}

void closeCover() {
  //digitalWrite(servoENA, LOW);
  myservo.attach(servoPin, 500, 2400);
  if (coverState != "CLOSED") {
    for (pos = 0; pos <= 180; pos++) {  // goes from 0 degrees to 180 degrees
      // in steps of 1 degree
      myservo.write(pos);  // tell servo to go to position in variable 'pos'
      delay(15);           // waits 15ms for the servo to reach the position
    }
    Serial.print("#");
    //  digitalWrite(servoENA, HIGH);

  }
  coverState = "CLOSED";
  analogWriteFrequency(ledENA, 1000);
  myservo.detach();
}

void openCover() {
  myservo.attach(servoPin, 500, 2400);
  // digitalWrite(servoENA, LOW);
  if (coverState != "OPEN") {
    for (pos = 180; pos >= 0; pos--) {  // goes from 180 degrees to 0 degrees
      myservo.write(pos);                  // tell servo to go to position in variable 'pos'
      delay(15);                           // waits 15ms for the servo to reach the position
    }
    Serial.print("#");
    // digitalWrite(servoENA, HIGH);
  }
  coverState = "OPEN";
  analogWriteFrequency(ledENA, 1000);
  myservo.detach();
}

void ledON (int P) {
  /// Reset freq change coused by ServoESP32 library
  analogWrite(ledENA, P);
  if (P == 0) ledState = "OFF";
  else ledState = "ON";
}

void initWiFi() {
  Serial.println("Initializing WiFi...");

  String val = spiffsRead("WL");
  String ssid = getValue(val, ';', 0);
  String pass = getValue(val, ';', 1);

  Serial.println("SSID: " + ssid + "  PASS: " + pass);

  WiFi.setHostname(hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.println("IP Address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed. Starting AP mode...");

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("STA Failed to configure");
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), pass.c_str());

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  }
}

void processSerialCommand() {
  if (!Serial.available()) return; // Exit if no data available

  String cmd = Serial.readStringUntil('#');
  cmd.trim(); // Remove any leading/trailing whitespace

  if (cmd == "heaterON") {
    digitalWrite(heaterENA, HIGH);
  } else if (cmd == "heaterOFF") {
    digitalWrite(heaterENA, LOW);
  } else if (cmd == "50") {
    // analogWrite(ledENA, 127);
  } else if (cmd == "100") {
    // analogWrite(ledENA, 256);
  } else if (cmd == "close") {
    closeCover();
  } else if (cmd == "open") {
    openCover();
  }
}

void handleRootRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.html", String(), false, processor);

  int paramsNr = request->params();
  String ssid, pass;
  bool writeToSPIFFS = false;

  for (int i = 0; i < paramsNr; i++) {
    const AsyncWebParameter* p = request->getParam(i);
    String name = p->name();
    String value = p->value();

    if (name == "led") {
      ledON(value.toInt() ? ledMax : 0);
    } else if (name == "heater") {
      digitalWrite(heaterENA, value.toInt() ? HIGH : LOW);
    } else if (name == "cover") {
      value.toInt() ? openCover() : closeCover();
    } else if (name == "SSID") {
      ssid = value;
    } else if (name == "PASS") {
      pass = value;
      writeToSPIFFS = true;
    }
  }

  if (writeToSPIFFS) {
    spiffsWrite("WL", ssid + ";" + pass);
  }
}

void handleLEDStatus(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/LEDstatus.htm", String(), false, processor);

  if (request->hasParam("led")) {
    String value = request->getParam("led")->value();
    ledON(value.toInt());
    ledState = value;
  }
}

void handleHeaterStatus(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/HEATERstatus.htm", String(), false, processor);

  if (request->hasParam("heater")) {
    digitalWrite(heaterENA, request->getParam("heater")->value().toInt() ? HIGH : LOW);
  }
}

void handleCoverStatus(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/COVERstatus.htm", String(), false, processor);

  if (request->hasParam("cover")) {
    request->getParam("cover")->value().toInt() ? openCover() : closeCover();
  }
}
