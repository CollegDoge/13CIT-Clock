#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <RTClib.h>
#include "DFRobotDFPlayerMini.h"

// PINS
const int trigPin = 11;
const int echoPin = 12;

const int MP3_RX_PIN = 3;
const int MP3_TX_PIN = 2;

const int BT_RX_PIN = 6;
const int BT_TX_PIN = 5;

// DEFINITIONS
Adafruit_SSD1306 display(128, 64, &Wire, -1);
DFRobotDFPlayerMini MP3player;
RTC_DS3231 rtc;

// SERIAL
SoftwareSerial SerialBT(BT_RX_PIN, BT_TX_PIN);
SoftwareSerial SerialMP3(MP3_RX_PIN, MP3_TX_PIN);

// LOGIC
unsigned long prevMillis = 0;
unsigned long menuNum = 1; 
unsigned long snoozeTimer = 0;
unsigned long alarmVolume = 20;

bool menuActive = false;
bool secondsEnabled = false;

bool isAlarmPlaying = false;
bool showSnooze = false;

// SETUP
void setup() {
  Serial.begin(9600);
  
  if (!rtc.begin()) {
    Serial.println(F("RTC Error"));
    setTime(0, 0, 0, 1, 1, 2026);
  } else {
    DateTime now = rtc.now();
    setTime(now.unixtime());
    Serial.println(F("RTC OK"));
  }

  SerialBT.begin(9600);
  SerialMP3.begin(9600);
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(15,28);
  display.print(F("Loading Clock :3"));
  display.display();
  delay(1000);
  adjustTime(1);

  SerialMP3.listen();
  if (MP3player.begin(SerialMP3, false, false)) {
    MP3player.volume(alarmVolume);
  }
  SerialBT.listen();
}

// MAIN LOOP
void loop() {
  unsigned long currentMillis = millis();

  // BT COMMANDS
  if (SerialBT.available()) {
    String msg = SerialBT.readStringUntil('\n');
    msg.trim();
    
    if (msg.equals("alarm")) {
      alarmPlay();
    } else if (msg.equals("menu")) {
      menuActive = !menuActive;
    } else if (msg.equals("seconds")) {
      secondsEnabled = !secondsEnabled;
    }

    if (msg.startsWith("V")) {
      long v = msg.substring(1).toInt();
      alarmVolume = v;
      Serial.println(F("Volume Changed"));
    }
    
    if (msg.startsWith("T")) {
      long t = msg.substring(1).toInt();
      setTime(t);
      rtc.adjust(DateTime(t));
      Serial.println(F("Time Synced via Phone"));
    }
  }

  // ULTRASONIC SENSOR (SNOOZE)
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
      triggerSnooze(currentMillis);
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

void alarmPlay() {
  SerialMP3.listen();
  MP3player.volume(alarmVolume);
  delay(50);
  MP3player.play(1);
  delay(50);
  SerialBT.listen();
  isAlarmPlaying = true;
}

void triggerSnooze(unsigned long currentMillis) {
  SerialMP3.listen();
  delay(50);
  MP3player.stop();
  isAlarmPlaying = false; 
  showSnooze = true;
  snoozeTimer = currentMillis;
  SerialBT.listen();
  displayTime();
}

void displayTime() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // MENU
  if (menuActive) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("CLOCK"));

    display.setCursor(40, 0);
    display.print(F("TIMER"));

    display.setCursor(80, 0);
    display.print(F("STPWATCH"));

    if (menuNum == 1) {
      display.drawLine(0, 9, 29, 9, WHITE);
    }
    if (menuNum == 2) {
      display.drawLine(40, 9, 68, 9, WHITE);
    }
    if (menuNum == 3) {
      display.drawLine(80, 9, 126, 9, WHITE);
    }
  }

  // CLOCK
  display.setTextSize(3);
  if (secondsEnabled) {
      display.setCursor(0, 20); 

      if (hour() < 10) display.print(F("0")); display.print(hour());
      display.print(F(":"));
      if (minute() < 10) display.print(F("0")); display.print(minute());
      
      display.setTextSize(2);
      display.print(F(" ")); 
      if (second() < 10) display.print(F("0")); display.print(second());
  } else {
    display.setCursor(20, 20); 
    
    if (hour() < 10) display.print(F("0")); display.print(hour());
    display.print(F(":"));
    if (minute() < 10) display.print(F("0")); display.print(minute());
  }

  // NOTIFICATIONS
  if (isAlarmPlaying) {
    display.setCursor(25, 55); display.setTextSize(1);
    display.print(F("ALARM ACTIVE"));
  }
  if (showSnooze) {
    display.setCursor(25, 55); display.setTextSize(1);
    display.print(F("ALARM STOPPED"));
  }
  display.display();
}