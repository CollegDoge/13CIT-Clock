#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>
#include "DFRobotDFPlayerMini.h"

// MISC
const int trigPin = 11;
const int echoPin = 12;

// DISPLAY
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MP3
const uint8_t PIN_MP3_TX = 2;
const uint8_t PIN_MP3_RX = 3;
DFRobotDFPlayerMini MP3player;

long duration;
float timing = 0.0;
int distance;

// BLUETOOTH
const int RX  = 6;
const int TX  = 5;
SoftwareSerial SerialBT(RX, TX);
SoftwareSerial SerialMP3(PIN_MP3_RX, PIN_MP3_TX);
String msg; 

// SETUP
void setup() {
  Serial.begin(9600);
  SerialBT.begin(9600);
  SerialMP3.begin(9600);
  SerialBT.println(F("Bluetooth connection  is established"));

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  delay(1000); // idk if i need this much delay
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 16);

  setTime(0, 0, 0, 1, 1, 2026);
  MP3player.begin(SerialMP3);
  MP3player.volume(30);
  MP3player.play(1);
}

void clock() {

}

void alarm() {

}

void stopwatch() {
  
}

void timer() {

}

void menu() {

}

// MAIN LOOP
void loop() {
  // TIME SYNC/BLUETOOTH
  SerialBT.listen();
  if (SerialBT.available()){
    msg = SerialBT.readStringUntil('\n');
    msg.trim();
    
    if (msg.startsWith("T")) {
      String timeString = msg.substring(1); 
      unsigned long pctime = timeString.toInt();
      
      if (pctime > 0) { 
        setTime(pctime);
        SerialBT.println(F("Time Synced!"));
      }
    }
    else if (msg == "test") {
        SerialBT.println(F("its working yay"));
    }
  }

  // FOR THE ULTRASONIC SENSOR
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  timing = pulseIn(echoPin, HIGH);
  distance = (timing * 0.034) / 2;
  
  // CLOCK
  static int lastSecond = -1;
  if (second() != lastSecond) {
    lastSecond = second();
    displayTime();
  }

  // temp for ultrasonic  
  delay(100);
}

void displayTime() {
  display.clearDisplay();
  display.setCursor(0, 16);
  display.setTextSize(3);
  
  if (hour() < 10) display.print("0");
  display.print(hour());
  display.print(":");
  
  if (minute() < 10) display.print("0");
  display.print(minute());
  display.setTextSize(2);
  display.print(" "); 

  if (second() < 10) display.print("0");
  display.print(second());

  display.display(); 
}