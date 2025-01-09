#include <ESP32Servo.h>

Servo myservo;  // create servo object to control a servo
// 16 servo objects can be created on the ESP32

int pos = 0;  // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33
int servoPin = 13;
int servoENA = 32;

const int potPin1 = 15;
const int potPin2 = 25;
const int resistorValue = 1000;  // Resistor value in ohms

int potValue1 = 0; // ntc na plocici
int potValue2 = 0;
unsigned long previousMillis = 0;  // will store last time LED was updated

int ledPWM = 0;
int ledENA = 26;

int heaterENA = 27;

// constants won't change:
const long interval = 1000;  // interval at which to blink (milliseconds)

void setup() {
  Serial.begin(9600);
  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);           // standard 50 hz servo
  myservo.attach(servoPin, 500, 2400);  // attaches the servo on pin 18 to the servo object
  // using default min/max of 1000us and 2000us
  // different servos may require different min/max settings
  // for an accurate 0 to 180 sweep
  pinMode(servoENA, OUTPUT);
  pinMode(ledENA, OUTPUT);
  pinMode(heaterENA, OUTPUT);
}

void loop() {

  String cmd;
  if (Serial.available() > 0) {
    // read the incoming byte:
    cmd = Serial.readStringUntil('#');
    if (cmd == "heaterON") {
      digitalWrite(heaterENA, HIGH);
    }
    if (cmd == "heaterOFF") {
      digitalWrite(ledENA, LOW);
    }
    if (cmd == "50") {
      analogWrite(ledENA, 127);
    }
    if (cmd == "100") {
      analogWrite(ledENA, 256);
    }

    if (cmd == "close") {
      digitalWrite(servoENA, HIGH);
      for (pos = 0; pos <= 180; pos += 1) {  // goes from 0 degrees to 180 degrees9
        // in steps of 1 degree
        myservo.write(pos);  // tell servo to go to position in variable 'pos'
        delay(15);           // waits 15ms for the servo to reach the position
      }
      Serial.print("#");
      digitalWrite(servoENA, LOW);
    }

    if (cmd == "open") {
      digitalWrite(servoENA, HIGH);
      for (pos = 180; pos >= 0; pos -= 1) {  // goes from 180 degrees to 0 degrees
        myservo.write(pos);                  // tell servo to go to position in variable 'pos'
        delay(15);                           // waits 15ms for the servo to reach the position
      }
    }
    digitalWrite(servoENA, LOW);
    Serial.print("#");
  }



  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    //Temp(analogRead(potPin1));
    
    Serial.println(analogRead(potPin1));

    //Temp(analogRead(potPin2));


    Serial.println("------------");
  }
}

float Temp(int sensorValue) {
  // Convert analog reading to resistance
  // float resistance = resistorValue / ((4096.0 / sensorValue) - 1.0);



  // Use the Steinhart-Hart equation to convert resistance to temperature in Celsius
  // float temperature = log(resistance / 10000.0) / 3950.0 + 1.0 / (25 + 273.15);


  float voltage = (sensorValue * 3.3) / 4095.0;
  float resistance = (voltage * 100000) / (3.3 - voltage);

  float tempK = 1 / (((log(resistance / 10000)) / 3950) + 1 / (25 + 273.15));

  float tempC = tempK - 273.15;

  Serial.print("Resistance: ");
  Serial.print(resistance);
  Serial.print("SensValue: ");
  Serial.print(sensorValue);
  Serial.print("  Temperature: ");
  Serial.print(tempC);
  Serial.print(" °C | ");
  return tempC;
}