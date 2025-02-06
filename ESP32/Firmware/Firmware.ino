#include "WiFi.h"
#include "ESPAsyncWebServer.h"
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
  ////Serial.println(var);
  if (var == "STATE") {

    return ledState;
  }

  if (var == "STATE2") {
    if (pos >= 180) { // treba zamijeniti s ServoENA
      coverState = "CLOSED";
    }
    else if (pos <= 0) {
      coverState = "OPEN";
    }
    else if (pos > 0 && pos < 180) coverState = "MOVING";
    return coverState;
  }

  if (var == "STATE3") {
    if (digitalRead(heaterENA)) { // treba zamijeniti s HeaterENA
      heaterState = "ON";
    }
    else {
      heaterState = "OFF";
    }
    //Serial.print(heaterState);
    return heaterState;
  }

  if (var == "SSID") {
    String val = getValue(spiffsRead("WL"), ';', 0);
    return val;
  }

  if (var == "PASS") {
    String val = getValue(spiffsRead("WL"), ';', 1);
    return val;
  }

  if (var == "iP") {
    String val = getValue(spiffsRead("WL"), ';', 2);
    return val;
  }

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

  // Definiranje html datoteka spremljenih u spiffsima
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);

    int paramsNr = request->params();
    bool Write = false;
    String var1;
    String var2;
    String var3;
    String var4;
    String var5;

    for (int i = 0; i < paramsNr; i++) {

      const AsyncWebParameter* p = request->getParam(i);

      if (p->name() == "led") {
        var1 = p->value();
        if (var1 == "1") ledON(ledMax);
        if (var1 == "0") ledON(0);
      }
      if (p->name() == "heater") {
        var2 = p->value();
        if (var2 == "1") digitalWrite(heaterENA, HIGH);
        if (var2 == "0") digitalWrite(heaterENA, LOW);
      }
      if (p->name() == "cover") {
        var3 = p->value();
        if (var3 == "1") openCover();
        if (var3 == "0") closeCover();
      }
      if (p->name() == "SSID") {
        var4 = p->value();
      }
      if (p->name() == "PASS") {
        var5 = p->value();
        Write = true;
      }
    }

    if (Write == true) {
      spiffsWrite("WL", var4 + ";" + var5);
      Write = false;
    }
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/jquery.min.js", "text/css");
  });
  server.on("/LEDstatus.htm", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/LEDstatus.htm", String(), false, processor);
    int paramsNr = request->params();
    String var1;

    for (int i = 0; i < paramsNr; i++) {

      const AsyncWebParameter* p = request->getParam(i);

      if (p->name() == "led") {
        var1 = p->value();
        ledON(var1.toInt());
        ledState = var1;

      }
    }
  });
  server.on("/HEATERstatus.htm", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/HEATERstatus.htm", String(), false, processor);

    int paramsNr = request->params();
    String var1;

    for (int i = 0; i < paramsNr; i++) {
      const AsyncWebParameter* p = request->getParam(i);

      if (p->name() == "heater") {
        var1 = p->value();
        if (var1 == "1") digitalWrite(heaterENA, HIGH);
        if (var1 == "0") digitalWrite(heaterENA, LOW);
      }
    }
  });
  server.on("/COVERstatus.htm", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/COVERstatus.htm", String(), false, processor);

    int paramsNr = request->params();
    String var1;

    for (int i = 0; i < paramsNr; i++) {
      const AsyncWebParameter* p = request->getParam(i);

      if (p->name() == "cover") {
        var1 = p->value();
        if (var1 == "1") {
          openCover(); 
        }
        if (var1 == "0") {
          closeCover();
        }
      }
    }
  });
  // Start server
  server.begin();
}

void loop() {
  //thermistorValue1 = analogRead(thermistorPin1);
  //Serial.println(thermistorValue1);
  //delay(500);

  // thermistorValue2 = analogRead(thermistorPin2);
  // Serial.println(thermistorValue2);
  //  delay(500);


  String cmd;
  // Serijska komunikacija
  if (Serial.available() > 0) {
    // read the incoming byte:
    cmd = Serial.readStringUntil('#');
    if (cmd == "heaterON") {
      digitalWrite(heaterENA, HIGH);
    }
    if (cmd == "heaterOFF") {
      digitalWrite(heaterENA, LOW);
    }
    if (cmd == "50") {
      //analogWrite(ledENA, 127);
    }
    if (cmd == "100") {
      // analogWrite(ledENA, 256);
    }

    if (cmd == "close") {
      closeCover();
    }

    if (cmd == "open") {
      openCover();
    }
  }
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
  // Ako se u 5 navrata ne uspije spojiti na WiFi, onda generira svoj AP gdje se mogu spojiti na web server
  int i = 0;
  String val = spiffsRead("WL");
  Serial.println("SSID: " + getValue(val, ';', 0) + "  PASS:" + getValue(val, ';', 1));

  WiFi.setHostname(hostname);

  WiFi.begin(string2char(getValue(val, ';', 0)), string2char(getValue(val, ';', 1)));
  while (WiFi.status() != WL_CONNECTED && i <= 5) {
    i++;
    delay(1000);
    Serial.println("Connecting to WiFi..");  // Print ESP32 Local IP Address
  }

  Serial.println(WiFi.localIP());
  if (i > 5) {

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("STA Failed to configure");
    }

    // Connect to Wi-Fi network with SSID and password
    Serial.print("Setting AP (Access Point)…");
    // Remove the password parameter, if you want the AP (Access Point) to be open

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  }
}
