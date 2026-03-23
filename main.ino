#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>

// MISC
const int trigPin = 11;
const int echoPin = 12;
const int buzzer = 2;

// DISPLAY
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

long duration;
float timing = 0.0;
int distance;

// BLUETOOTH
const int RX  = 6;
const int TX  = 5;
SoftwareSerial SerialBT(RX,  TX);
String msg; 

// SETUP
void setup() {
  Serial.begin(9600);
  SerialBT.begin(9600);
  SerialBT.println("Bluetooth connection  is established");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzer, OUTPUT);

  digitalWrite(buzzer, LOW);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  delay(1000); // idk if i need this much delay
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0, 16);

  setTime(9, 30, 0, 1, 1, 2024); 
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
  if (SerialBT.available()){
    msg = SerialBT.readStringUntil('\n');
    msg.trim();
    Serial.print("Received: "); // Print to computer for debugging
    Serial.println(msg);

    if (msg == "test") {
        SerialBT.println("its working yay");
    }
    else {
        SerialBT.println("Command '");
        SerialBT.print(msg);
        SerialBT.print("'  is unknown");
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
  
  // CLOCK FUNC
  static int lastSecond = -1;
  if (second() != lastSecond) {
    lastSecond = second();
    displayTime();
  }

  if (distance <= 10) {
    tone(buzzer, 500);
  } else {
    noTone(buzzer);
  }
  
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