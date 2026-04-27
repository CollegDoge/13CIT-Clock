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
Adafruit_SSD1306 display(128, 64, &Wire, -1); // define display resolution/name
DFRobotDFPlayerMini MP3player; // define MP3 player name
RTC_DS3231 rtc; // define RTC module name

// SERIAL
SoftwareSerial SerialBT(BT_RX_PIN, BT_TX_PIN); // define BT name/pins used
SoftwareSerial SerialMP3(MP3_RX_PIN, MP3_TX_PIN); // define MP3 name/pins used

// LOGIC
unsigned long prevMillis = 0; // so i can avoid using delay.
bool isAlarmPlaying = false; // if the alarm is playing
char buffer[12]; // character buffer for bluetooth (12 is enough for what im doing)

int alarmHour = -1; // set alarm hour value. -1 is not set
int alarmMinute = -1; // set alarm minute value. -1 is not set
bool alarmEnabled = false; // if the alarm is enabled.
uint8_t alarmVolume = 20; // the volume of the alarm
uint8_t alarmTone = 1; // what MP3 it uses.

bool secondsEnabled = false; // if seconds should be shown on the display
bool dateActive = false; // if the date should be shown on the display
 
unsigned long snoozeTimer = 0; // snooze message delay
bool showSnooze = false; // if the snooze message is shown

// SETUP
void setup() {
  Serial.begin(9600); // begin serial monitor
  
  if (!rtc.begin()) { // sync time with phone, otherwise set a backup date/time.
    Serial.println(F("RTC Error"));
    setTime(0, 0, 0, 1, 1, 2026);
  } else {
    DateTime now = rtc.now();
    setTime(now.unixtime());
    Serial.println(F("RTC OK"));
  }

  // begin BT/MP3 serial
  SerialBT.begin(9600);
  SerialMP3.begin(9600);
  
  // for the ultrasonic sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // if display filed to start. mainly for testing, dont need.
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  // set up display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(15,28);
  display.print(F("Loading Clock :3"));
  display.display();
  delay(1000);
  adjustTime(1); // add 1 second as we have just used delay. kinda chopped.

  // start MP3 first, set default volume.
  SerialMP3.listen();
  if (MP3player.begin(SerialMP3, false, false)) {
    MP3player.volume(alarmVolume);
  }
  // start BT next.
  SerialBT.listen();
}

// MAIN LOOP
void loop() {
  unsigned long currentMillis = millis(); // so we can aboid delay where possible.

  // BT COMMANDS
  if (SerialBT.available()) {
    int len = SerialBT.readBytesUntil('\n', buffer, sizeof(buffer) - 1); // max length of 12
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
    else if (buffer[0] == 'V') { // change volume based of value 1-30
      alarmVolume = atoi(buffer + 1);
      Serial.println(F("Volume Changed"));
    }
    else if (buffer[0] == 'T') { // sets the time using unix timestamp.
      long t = atol(buffer + 1);
      setTime(t);
      rtc.adjust(DateTime(t));
      Serial.println(F("Time Synced via Phone"));
    }
    else if (buffer[0] == 'A') { // sets alarm (a0 = unset, axxxx = set)
      int val = atoi(buffer + 1);

      if (val == 0) {
        alarmEnabled = false;
        Serial.println(F("Alarm Disabled"));
      } else {
        alarmHour = val / 100;
        alarmMinute = val % 100;
        alarmEnabled = true;

        // for testing
        Serial.print(F("Alarm Set: "));
        Serial.print(alarmHour);
        Serial.print(F(":"));
        Serial.println(alarmMinute);
      }
    }
  }

  // ULTRASONIC SENSOR (SNOOZE)
  if (currentMillis - prevMillis >= 150) { // if distance is under 150, activate
    prevMillis = currentMillis;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2); // lowest delay i could get away with.
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH, 30000);
    int distance = (duration * 0.034) / 2;

    // actually snooze if an alarm is playing.
    if (distance > 0 && distance < 20 && isAlarmPlaying) {
      triggerSnooze(currentMillis);
    }
  }

  // SNOOZE DISPLAY TIMER
  if (showSnooze && (currentMillis - snoozeTimer >= 3000)) { // show snooze msg for 3 seconds.
    showSnooze = false;
    displayTime();
  }

  // CLOCK REFRESH
  static int lastSec = -1;
  if (second() != lastSec) { // refresh every second.
    lastSec = second();
    displayTime();
  }
}

void alarmPlay() {
  SerialMP3.listen(); // activate mp3 listener
  MP3player.volume(alarmVolume); // set volume if changed
  MP3player.play(alarmTone); // play alarm
  SerialBT.listen(); // reactivate bt listener
  isAlarmPlaying = true;
}

void triggerSnooze(unsigned long currentMillis) { // INTERRUPT
  SerialMP3.listen(); // activate mp3 listener
  MP3player.stop(); // stop alarm
  isAlarmPlaying = false; 
  showSnooze = true;
  snoozeTimer = currentMillis;
  SerialBT.listen(); // reactivate bt listener
  displayTime();
}

void displayTime() {
  display.clearDisplay();

  // DATE
  if (dateActive) { // show date if enabled
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

  if (alarmEnabled && !isAlarmPlaying) { // if the hour/minute match, activate the alarm
    if (hour() == alarmHour && minute() == alarmMinute) {
      if (lastAlarmMinute != minute()) {
        alarmPlay();
        lastAlarmMinute = minute();
      }
    }
  }

  if (minute() != lastAlarmMinute) { // dont reactivate once snoozed
    lastAlarmMinute = -1;
  }

  // CLOCK
  display.setTextSize(3); // big text
  if (secondsEnabled) { // if seconds enabled, add them on the end
      display.setCursor(0, 20); 

      if (hour() < 10) display.print(F("0")); display.print(hour()); // add 0 if hour is under 10.
      display.print(F(":"));
      if (minute() < 10) display.print(F("0")); display.print(minute());
      
      display.setTextSize(2);
      display.print(F(" ")); 
      if (second() < 10) display.print(F("0")); display.print(second());
  } else { // regular (no seconds) display mode
    display.setCursor(20, 20); 
    
    if (hour() < 10) display.print(F("0")); display.print(hour());
    display.print(F(":"));
    if (minute() < 10) display.print(F("0")); display.print(minute());
  }

  // NOTIFICATIONS
  if (isAlarmPlaying) { // display alarm active message at bottom of display
    display.setCursor(25, 55); display.setTextSize(1);
    display.print(F("ALARM ACTIVE"));
  }
  if (showSnooze) { // display alarm stop message at bottom of display
    display.setCursor(25, 55); display.setTextSize(1);
    display.print(F("ALARM STOPPED"));
  }
  display.display();
}