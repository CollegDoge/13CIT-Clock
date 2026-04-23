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
bool isAlarmPlaying = false;
char buffer[12];

int alarmHour = -1;
int alarmMinute = -1;
bool alarmEnabled = false;
uint8_t alarmVolume = 20;
uint8_t alarmTone = 1;

bool secondsEnabled = false;
bool dateActive = false;

unsigned long snoozeTimer = 0;
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
    int len = SerialBT.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';

    // STRING MATCH CMDS
    if (strcmp(buffer, "alarm") == 0) {
      alarmPlay();
    }
    else if (strcmp(buffer, "date") == 0) {
      dateActive = !dateActive;
    }
    else if (strcmp(buffer, "seconds") == 0) {
      secondsEnabled = !secondsEnabled;
    }
    else if (strcmp(buffer, "tone") == 0) {
      alarmTone = (alarmTone == 1) ? 2 : 1;
    }

    // STRING START CMDS
    else if (buffer[0] == 'V') {
      alarmVolume = atoi(buffer + 1);
      Serial.println(F("Volume Changed"));
    }
    else if (buffer[0] == 'T') {
      long t = atol(buffer + 1);
      setTime(t);
      rtc.adjust(DateTime(t));
      Serial.println(F("Time Synced via Phone"));
    }
    else if (buffer[0] == 'A') {
      int val = atoi(buffer + 1);

      if (val == 0) {
        alarmEnabled = false;
        Serial.println(F("Alarm Disabled"));
      } else {
        alarmHour = val / 100;
        alarmMinute = val % 100;
        alarmEnabled = true;

        Serial.print(F("Alarm Set: "));
        Serial.print(alarmHour);
        Serial.print(F(":"));
        Serial.println(alarmMinute);
      }
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
  MP3player.play(alarmTone);
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

  // DATE
  if (dateActive) {
    display.setCursor(36, 0);
    display.setTextSize(1);
    display.print(day());
    display.print(F("/"));
    display.print(month());
    display.print(F("/"));
    display.print(year());
  }

  // ALARM
  static int lastAlarmMinute = -1;

  if (alarmEnabled && !isAlarmPlaying) {
    if (hour() == alarmHour && minute() == alarmMinute) {
      if (lastAlarmMinute != minute()) {
        alarmPlay();
        lastAlarmMinute = minute();
      }
    }
  }

  if (minute() != lastAlarmMinute) {
    lastAlarmMinute = -1;
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