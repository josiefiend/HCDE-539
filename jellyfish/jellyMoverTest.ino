#include <Servo.h>

Servo jellyMover; // servo to control jellyfish
int position; // servo position
const int hallSensorPin = 7; // magnet sensor is attached here
int predatorDetected;

void setup() {
  
  pinMode(hallSensorPin, INPUT);  // set sensor pin as input
  jellyMover.attach(9); // set jellyfish on pin 9
}

void loop() {
  // put your main code here, to run repeatedly:
  // read magnet sensor input
  int hallEffectSensorValue = digitalRead(hallSensorPin);
  // the sensor turns voltage off (returns 0) when a magnet is present
  predatorDetected = (hallEffectSensorValue == 0);
  Serial.println(predatorDetected);
  if (predatorDetected) {
    swayJellyfish();
  }
}

//TODO: detectPredator, detectAlert, wakeJellyfish, colorJellyfish 


// jellyfish moves
void swayJellyfish() {
  // sweep the servo with the jellyfish attached
  // TODO: jiggle the servo with the jellyfish attached
  for (position = 0; position <= 180; position += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    jellyMover.write(position);              // send servo to position
    delay(15);
  }
  for (position = 180; position >= 0; position -= 1) { // goes from 180 degrees to 0 degrees
    jellyMover.write(position);              // send servo to position
    delay(15);
  }
}
