/* 539 Jellyfish
   joeyhoy@uw.edu
   Uses AlertNodeLib by ahdavidson, LCD library by Adafruit, Servo library from Arduino, FastLED
*/

#include <SoftwareSerial.h>
#include <Servo.h>
#include <Adafruit_LiquidCrystal.h>
#include <AlertNodeLib.h>
#include <bitswap.h>
#include <chipsets.h>
#include <color.h>
#include <colorpalettes.h>
#include <colorutils.h>
#include <controller.h>
#include <cpp_compat.h>
#include <dmx.h>
#include <FastLED.h>
#include <fastled_config.h>
#include <fastled_delay.h>
#include <fastled_progmem.h>
#include <fastpin.h>
#include <fastspi.h>
#include <fastspi_bitbang.h>
#include <fastspi_dma.h>
#include <fastspi_nop.h>
#include <fastspi_ref.h>
#include <fastspi_types.h>
#include <hsv2rgb.h>
#include <led_sysdefs.h>
#include <lib8tion.h>
#include <noise.h>
#include <pixelset.h>
#include <pixeltypes.h>
#include <platforms.h>
#include <power_mgt.h>1

//const int debugPin = LED_BUILTIN; // set built in LED for debugging - commented b/c I'm currently using pin 13 in my build
const int hallSensorPin = 7; // magnet sensor is attached here
const int photoSensorPin = A0; // connect sensor to analog pin
const int tempSensorPin = A1; // connect sensor to analog pin
// settings for AlertLib
const String myNodeName = "Jellyfish"; // name node for use with AlertNodeLib
const boolean SENDING = true;
const boolean RECEIVING = true;
boolean myDebugging = false; // change this based on your preference
// settings for FastLED with APA102 DotStar strip
const int DATA_PIN = 5; // TODO: update for multiple strips
const int CLK_PIN = 13; // TODO: update for multiple strips
#define LED_TYPE APA102
#define COLOR_ORDER BGR
#define NUM_LEDS 144
int BRIGHTNESS = 96;
const int FRAMES_PER_SECOND = 120;
CRGB leds[NUM_LEDS];

Servo jellyMover; // servo to control jellyfish
Adafruit_LiquidCrystal lcd(0); // initialize LCD
AlertNode myNode; // connect to XBee; pin 2 TX and pin 3 for RX, 9600 baud

int position; // servo position
int predatorDetected; // state of predators
int photoSensorValue; // amount of light
float voltageValueForTemp; // voltage reading from temperature sensor
float tempC; // temperature in celsius
float tempF; // temperature in fahrenheit

// alert timers & states
unsigned long currentTime; // set in main loop
unsigned long lastLoopStartTime = millis();
long loopDelta;
long earthquakeOnRemainingTime; // counter for alert
long floodOnRemainingTime;
long oceanAcidificationOnRemainingTime;
long jellyWarningOnRemainingTime;
bool earthquakeOn; // state of alert
bool floodOn;
bool oceanAcidificationOn;
bool jellyWarningOn;
int currentAlert = AlertNode::NO_ALERT;
String messageLn1;
String messageLn2;
bool needsLight;
bool tooHot;

void setup() {
  delay(3000); // 3 second recommended delay for recovery for FastLED
  Serial.begin(9600);
  myNode.begin(myNodeName); // start node
  Serial.print("\n\n*** STARTING NODE ");
  Serial.println(myNodeName);
  myNode.setDebug(false); // configurable debugging state

  jellyMover.attach(4); // set jellyfish servo on pin 4
  pinMode(hallSensorPin, INPUT);  // set digital sensor pins as input

  // setup for FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // set brightness control
  FastLED.setBrightness(BRIGHTNESS);

  sleepJellyfish(); // ensure that LED strip is off on startup
}

void loop() {

  FastLED.delay(1000 / FRAMES_PER_SECOND); // insert a delay for FastLED to keep the framerate modest
  // timer
  currentTime = millis();
  loopDelta = currentTime - lastLoopStartTime;
  lastLoopStartTime = currentTime;

  // check temperature; if it's too hot, the jellyfish will be lethargic and its lights will dim
  senseTemperature();
  if (tooHot) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeLightBy(128); // uses FastLED's dim
    }
  }

  senseLight(); // check the light level, which will be used for logic in default jellyfish alert state wakeJellyfish();

  // read magnet sensor input
  detectPredator();
  if (predatorDetected) {
    warnOtherJellyfish(); // tell jelly friends
  }

  currentAlert = TransitionAlertMode(currentAlert); // TransitionAlertMode sets states for each alert, returns current alert mode
  // Transition between alerts
  switch (currentAlert) {
    case AlertNode::EARTHQUAKE:
      currentAlert = earthquakeRender();
      break;
    case AlertNode::FLOOD:
      currentAlert = floodRender();
      break;
    case AlertNode::OCEAN_ACIDIFICATION:
      currentAlert = oceanAcidificationRender();
      break;
    case AlertNode::JELLY_WARNING:
      currentAlert = jellyWarningRender();
      break;
    default: // jellyfish behavior when no alert is present
      wakeJellyfish(); // checks light level state and wakes up if it light is needed
      break;
  }
}

// JELLYFISH BEHAVIOR FUNCTIONS

void detectPredator() {
  int hallEffectSensorValue = digitalRead(hallSensorPin);
  // the sensor turns voltage off (returns 0) when a magnet is present
  predatorDetected = (hallEffectSensorValue == 0);
}

void senseLight() {
  photoSensorValue = analogRead(photoSensorPin); // read photo sensor input
  if (photoSensorValue < 200) {
    needsLight = true;
    Serial.println(needsLight);
  }
  else needsLight = false;
}

void senseTemperature() {
  voltageValueForTemp = (analogRead(tempSensorPin) * 0.004882814); // read temperature sensor input
  tempC = (voltageValueForTemp - 500) / 10; // this uses voltage, convert to a temperature (common formula)
  tempF = tempC * (9.0 / 5.0) + 32.0; // convert celsius to fahrenheit
  //Serial.println(tempF);
}

// FastLED  pattern function - from FastLED demo reel
void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS - 1);
  leds[pos] += CHSV(172, 255, 255); // purple
}

void wakeJellyfish() {
  if (needsLight) {
    sinelon(); // default jellyfish pattern from FastLED
  }
  else sleepJellyfish();
}

void sleepJellyfish() {
  FastLED.clear(); // turns off LED strip
}

void swayJellyfish() {
  // sweep the servo with the jellyfish attached
  for (position = 0; position <= 180; position += 1) { // goes from 0 degrees to 180 degrees in steps of 1 degree
    jellyMover.write(180); // send servo to position
    // Serial.println(position);
  }
  for (position = 180; position >= 0; position -= 1) { // goes from 180 degrees to 0 degrees
    jellyMover.write(0); // send servo to position
    // Serial.println(position);
  }
}

// ALERT HANDLING FUNCTIONS

// capture alert, set up timer start state for each
int TransitionAlertMode(int previousAlertMode)
{
  int alert = myNode.alertReceived();
  switch (alert) {
    case AlertNode::EARTHQUAKE:
      earthquakeSetup();
      break;
    case AlertNode::FLOOD:
      floodSetup();
      break;
    case AlertNode::OCEAN_ACIDIFICATION:
      oceanAcidificationSetup();
      break;
    case AlertNode::JELLY_WARNING:
      jellyWarningSetup();
      break;
    case AlertNode::NO_ALERT:
      // nothing changed
      return previousAlertMode;
  }
  return alert;
}

void earthquakeSetup() {
  earthquakeOnRemainingTime = 3000; // set current alert to run for 3 seconds
  floodOnRemainingTime = 0; // set other alerts to zero time to run
  oceanAcidificationOnRemainingTime = 0;
  jellyWarningOnRemainingTime = 0;
  earthquakeOn = false; // start this alert in off state
  messageLn1 = "Shake shake!";
  messageLn2 = "Slosh";
}

void floodSetup() {
  earthquakeOnRemainingTime = 0;
  floodOnRemainingTime = 3000;
  oceanAcidificationOnRemainingTime = 0;
  jellyWarningOnRemainingTime = 0;
  floodOn = false; // start this alert in off state
  messageLn1 = "Rising waters!";
  messageLn2 = "Space to swim!";
}

void oceanAcidificationSetup() {
  earthquakeOnRemainingTime = 0;
  floodOnRemainingTime = 0;
  oceanAcidificationOnRemainingTime = 5000; // I set this longer so that I can coordinate with a wordy LCD printout for readability
  jellyWarningOnRemainingTime = 0;
  oceanAcidificationOn = false; // start this alert in off state
  messageLn1 = "CLIMATE CHANGE";
  messageLn2 = "IS REAL";
}

void jellyWarningSetup() {
  earthquakeOnRemainingTime = 0;
  floodOnRemainingTime = 0;
  oceanAcidificationOnRemainingTime = 0;
  jellyWarningOnRemainingTime = 3000;
  jellyWarningOn = false; // start this alert in off state
  messageLn1 = "Predator nearing!";
  messageLn2 = "Thanks, jelly friend";
}

int earthquakeRender() {
  if (!earthquakeOn) {
    earthquakeOn = true;
    printAlert(currentAlert);
  }
  fill_solid(leds, NUM_LEDS, CRGB::Yellow); // set all to the chosen color
  FastLED.show(); // write setting to LEDs
  swayJellyfish();
  earthquakeOnRemainingTime -= loopDelta; // count down
  if (earthquakeOnRemainingTime <= 0) {
    earthquakeOn = false;
    lcd.clear();
    return AlertNode::NO_ALERT; // when timer runs out, return default state to main loop
  }
  return AlertNode::EARTHQUAKE;
}

int floodRender() {
  if (!floodOn) {
    floodOn = true;
    printAlert(currentAlert);
  }
  fill_solid(leds, NUM_LEDS, CRGB::DarkBlue); // Set all to the chosen color
  FastLED.show();
  floodOnRemainingTime -= loopDelta;
  if (floodOnRemainingTime <= 0) {
    floodOn = false;
    lcd.clear();
    return AlertNode::NO_ALERT;
  }
  return AlertNode::FLOOD;
}

int oceanAcidificationRender() {
  if (!oceanAcidificationOn) {
    oceanAcidificationOn = true;
    printAlert(currentAlert);
  }
  fill_solid(leds, NUM_LEDS, CRGB::Maroon); // Set all to the chosen color
  FastLED.show();
  oceanAcidificationOnRemainingTime -= loopDelta;
  if (oceanAcidificationOnRemainingTime <= 0) {
    oceanAcidificationOn = false;
    lcd.clear();
    return AlertNode::NO_ALERT;
  }
  return AlertNode::OCEAN_ACIDIFICATION;
}

int jellyWarningRender() {
  if (!jellyWarningOn) {
    jellyWarningOn = true;
    printAlert(currentAlert);
  }
  fill_solid(leds, NUM_LEDS, CRGB::Chartreuse); // Set all to the chosen color
  FastLED.show();
  jellyWarningOnRemainingTime -= loopDelta;
  if (jellyWarningOnRemainingTime <= 0) {
    jellyWarningOn = false;
    lcd.clear();
    return AlertNode::NO_ALERT;
  }
  return AlertNode::JELLY_WARNING;
}

// send a special warning to other jellyfish on the node
void warnOtherJellyfish() {
  myNode.sendAlert(AlertNode::JELLY_WARNING);
  fill_solid(leds, NUM_LEDS, CRGB::White);   // flash white
  FastLED.show(); // write setting to LEDs
}

// WRITE MESSAGE TO LCD
void printAlert (int currentAlert) {
  int thisAlert = currentAlert;
  lcd.begin(16, 2); // set number of columns and rows on LCD
  lcd.setCursor(0, 0); // set position to write next line
  lcd.print(messageLn1);
  lcd.setCursor(0, 1); // set position to write next line
  lcd.print(messageLn2);
}


