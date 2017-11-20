/* 539 Jellyfish
   joeyhoy@uw.edu
   Uses AlertNodeLib and Button library by ahdavidson, LCD library by Adafruit, Servo library from Arduino
*/

#include <Servo.h>
#include <Adafruit_LiquidCrystal.h>
#include <AlertNodeLib.h>

const String myNodeName = "Jellyfish"; // name node for use with AlertNodeLib
const int debugPin = LED_BUILTIN; // set built in LED for debugging
const int hallSensorPin = 7; // magnet sensor is attached here
const int photoSensorPin = A0; // connect sensor to analog pin
const int redLED = 6;
const int greenLED = 5;
const int blueLED = 4;
const int redPin = 9;
const int greenPin = 10;
const int bluePin = 11;

Servo jellyMover; // servo to control jellyfish
Adafruit_LiquidCrystal lcd(0); // initialize LCD
AlertNode myNode; // connect to XBee; pin 2 TX and pin 3 for RX, 9600 baud

int position; // servo position
int predatorDetected = 0; // state of predators
int photoSensorValue = 0; // amount of light

void setup() {

  pinMode(hallSensorPin, INPUT);  // set sensor pin as input
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(redLED, OUTPUT); // rgb LED
  pinMode(greenLED, OUTPUT); // rgb LED
  pinMode(blueLED, OUTPUT); // rgb LED
  jellyMover.attach(2); // set jellyfish on pin 2
  myNode.begin(myNodeName); // start node
  analogWrite(greenPin, 255);
  analogWrite(redPin, 255);
  analogWrite(bluePin, 255);
}

void loop() {
  // put your main code here, to run repeatedly:

  // read magnet sensor input
  int alert = myNode.alertReceived();
  senseLight();
  detectPredator();
  if (predatorDetected) {
    wakeJellyfishRGB();
    wakeJellyfishLED();
    swayJellyfish();
  }
  sleepJellyfishRGB();
}

//TODO: detectAlert(), colorJellyfish(), brightenJellyfish(), writeLCD()


//TO DO: convert this to flight time sensor input when part comes in
void detectPredator() {
  int hallEffectSensorValue = digitalRead(hallSensorPin);
  // the sensor turns voltage off (returns 0) when a magnet is present
  predatorDetected = (hallEffectSensorValue == 0);
  Serial.println(predatorDetected);
}

//TO DO: convert this to new fancy light sensor when part comes in
void senseLight() {
  photoSensorValue = analogRead(photoSensorPin); // read photo sensor input
  Serial.println(photoSensorValue);
}

// UPDATE: added RGB wake, leaving these on breadboard for now
// TO DO: update this to set a default wake color transition (e.g. bright green, then fade to aqua)
void wakeJellyfishLED() {
  // flash R, G, and B LEDs
  digitalWrite(redLED, HIGH);
  digitalWrite(blueLED, HIGH);
  digitalWrite(greenLED, HIGH);
  delay(1000);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, LOW);
  digitalWrite(greenLED, LOW);
  delay(1000);
  digitalWrite(redLED, HIGH);
  digitalWrite(blueLED, HIGH);
  digitalWrite(greenLED, HIGH);
  delay(200);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, LOW);
  digitalWrite(greenLED, LOW);
  delay(200);
  digitalWrite(redLED, HIGH);
  digitalWrite(blueLED, HIGH);
  digitalWrite(greenLED, HIGH);
  delay(200);
  digitalWrite(redLED, LOW);
  digitalWrite(blueLED, LOW);
  digitalWrite(greenLED, LOW);
  delay(200);
}

// TO DO: update this to set a default wake color transition (e.g. bright green, then fade to aqua)
// currently cycles through red, green, blue, then purple, flashes three times, back to purple
void wakeJellyfishRGB() {
  analogWrite(redPin, 255);
  analogWrite(greenPin, 0);
  analogWrite(bluePin, 0);
  delay(500);
  analogWrite(redPin, 0);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 0);
  delay(500);
  analogWrite(redPin, 0);
  analogWrite(greenPin, 0);
  analogWrite(bluePin, 255);
  delay(500);
  analogWrite(redPin, 175);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 175);
  delay(500);
  analogWrite(redPin, 255);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 255);
  delay(100);
  analogWrite(redPin, 175);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 175);
  delay(100);
  analogWrite(redPin, 255);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 255);
  delay(100);
  analogWrite(redPin, 175);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 175);
  delay(100);
  analogWrite(redPin, 255);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 255);
  delay(100);
  analogWrite(redPin, 175);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 175);
  delay(100);
  analogWrite(redPin, 255);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 255);
  delay(500);
  analogWrite(redPin, 175);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 175);
  delay(500);
}

// turns off RGB
void sleepJellyfishRGB() {
  analogWrite(redPin, 255);
  analogWrite(greenPin, 255);
  analogWrite(bluePin, 255);
}

// jellyfish moves
void swayJellyfish() {
  // TODO: jiggle the servo with the jellyfish attached
  // sweep the servo with the jellyfish attached
  for (position = 0; position <= 180; position += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    jellyMover.write(position);              // send servo to position
    Serial.println(position);
    delay(15);
  }
  for (position = 180; position >= 0; position -= 1) { // goes from 180 degrees to 0 degrees
    jellyMover.write(position);              // send servo to position
    Serial.println(position);
    delay(15);
  }
}

void logAlert (String myName, int alert) {
  //TODO: revisit this, most of it is from alarm
  Serial.print("ALERT ");
  float sec = millis() / 1000.0;
  Serial.println(sec);
  Serial.print(myName);
  Serial.print(" | alert = ");
  Serial.print(alert);
  Serial.print(": ");
  Serial.println(myNode.alertName(alert));

  lcd.begin(16, 2); // set number of columns and rows on LCD
  lcd.print("Jellyfish "); // print mode to first line
  lcd.setCursor(0, 1); // set position to write next line
  lcd.print(myNode.alertName(alert) + " DETECTED");
}

