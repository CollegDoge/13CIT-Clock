#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>
#include "DFRobotDFPlayerMini.h"

// PINS
const int trigPin = 11;
const int echoPin = 12;

const int MP3_RX_PIN = 3;
const int MP3_TX_PIN = 2;

const int BT_RX_PIN = 6;
const int BT_TX_PIN = 5;

// DISPLAY
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// SERIAL
SoftwareSerial SerialBT(BT_RX_PIN, BT_TX_PIN);
SoftwareSerial SerialMP3(MP3_RX_PIN, MP3_TX_PIN);
DFRobotDFPlayerMini MP3player;

// LOGIC
unsigned long prevMillis = 0;
bool isAlarmPlaying = false;
bool showSnooze = false;
unsigned long snoozeTimer = 0;

// SETUP
void setup() {
  Serial.begin(9600);
  SerialBT.begin(9600);
  SerialMP3.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  setTime(0, 0, 0, 1, 1, 2026);

  SerialMP3.listen();
  if (MP3player.begin(SerialMP3, false, false)) {
    MP3player.volume(30);
  }
  SerialBT.listen();
}

void alarmPlay() {
  Serial.println(F("Starting Alarm..."));
  
  SerialMP3.listen();
  delay(50);
  MP3player.play(1);
  delay(50);
  
  SerialBT.listen();
  isAlarmPlaying = true;
}

void loop() {
  unsigned long currentMillis = millis();

  // BT COMMANDS
  if (SerialBT.available()) {
    String msg = SerialBT.readStringUntil('\n');
    msg.trim();
    if (msg.equals("alarm")) {
      alarmPlay();
    }
    if (msg.startsWith("T")) setTime(msg.substring(1).toInt());
  }

  // ULTRASONIC SENSOR
  if (currentMillis - prevMillis >= 150) {
    prevMillis = currentMillis;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000);
    int distance = (duration * 0.034) / 2;

    if (distance > 0 && distance < 20 && isAlarmPlaying) {
      Serial.println(F("Snooze Triggered"));
      
      SerialMP3.listen();
      delay(50);
      MP3player.stop();
      
      isAlarmPlaying = false; 
      showSnooze = true;
      snoozeTimer = currentMillis;
      
      SerialBT.listen();
      displayTime();
    }
  }

  // SNOOZE DISPLAY TIMER
  if (showSnooze && (currentMillis - snoozeTimer >= 3000)) {
    showSnooze = false;
    displayTime();
  }

  // CLOCK REFRESH
  static int lastSec = -1;
  if (second() != lastSec) {
    lastSec = second();
    displayTime();
  }
}

void displayTime() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  display.setCursor(0, 15); 
  display.setTextSize(3);
  if (hour() < 10) display.print("0"); display.print(hour());
  display.print(":");
  if (minute() < 10) display.print("0"); display.print(minute());
  
  display.setTextSize(2);
  display.print(" "); 
  if (second() < 10) display.print("0"); display.print(second());

  if (isAlarmPlaying) {
    display.setTextSize(1);
    display.setCursor(25, 52);
    display.print(F("ALARM PLAYING"));
  }
  if (showSnooze) {
    display.setTextSize(1);
    display.setCursor(25, 52);
    display.print(F("ALARM STOPPED"));

  }
  display.display();
}