
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
// settings for AlertLib
const String myNodeName = "Jellyfish"; // name node for use with AlertNodeLib
const boolean SENDING = true;
const boolean RECEIVING = true;
boolean myDebugging = true; // change this based on your preference
// settings for FastLED with APA102 DotStar strip
const int DATA_PIN = 5; // TODO: update for multiple strips
const int CLK_PIN = 13; // TODO: update for multiple strips
#define LED_TYPE APA102
#define COLOR_ORDER BGR
#define NUM_LEDS 100
const int BRIGHTNESS = 96;
const int FRAMES_PER_SECOND = 120;
CRGB leds[NUM_LEDS];

Servo jellyMover; // servo to control jellyfish
Adafruit_LiquidCrystal lcd(0); // initialize LCD
AlertNode myNode; // connect to XBee; pin 2 TX and pin 3 for RX, 9600 baud

int position; // servo position
int predatorDetected; // state of predators
int photoSensorValue; // amount of light

// alert timers & states
unsigned long lastLoopStartTime = millis();
unsigned long currentTime;
long loopDelta;
long earthquakeOnRemainingTime; // counter for alert
long floodOnRemainingTime;
long oceanAcidificationOnRemainingTime;
long jellyWarningOnRemainingTime;
bool earthquakeOn;
bool floodOn;
bool oceanAcidificationOn;
bool jellyWarningOn;
int currentAlert = AlertNode::NO_ALERT;

void setup() {
  delay(3000); // 3 second recommended delay for recovery for FastLED
  Serial.begin(9600);
  myNode.begin(myNodeName); // start node
  Serial.print("\n\n*** Starting AlertNodeBasic demo: ");
  Serial.println(myNodeName);
  //myNode.setDebug(true);

  jellyMover.attach(4); // set jellyfish on pin 4
  pinMode(hallSensorPin, INPUT);  // set sensor pin as input

  // setup for FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, COLOR_ORDER, DATA_RATE_MHZ(12)>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // set brightness control
  FastLED.setBrightness(BRIGHTNESS);

  sleepJellyfish();
}

void loop() {
  // put your main code here, to run repeatedly:

  FastLED.delay(1000 / FRAMES_PER_SECOND); // insert a delay for FastLED to keep the framerate modest
  // timer
  currentTime = millis();
  loopDelta = currentTime - lastLoopStartTime;
  lastLoopStartTime = currentTime;

  //TO DO: REINTEGRATE THIS WITH NEW STRUCTURE
  // 1. JELLYFISH CREATES LIGHT WHEN IT'S DARK
  //      senseLight();
  //      if (photoSensorValue > 200) {
  //        wakeJellyfish();
  //      }
  //      else if (photoSensorValue > 200) {
  //        sleepJellyfish(); //if it's bright, turn off LEDs
  //      }

  // 2. JELLYFISH RESPONDS TO ALERTS
  currentAlert = TransitionAlertMode(currentAlert); // TransitionAlertMode sets states for each alert, returns current alert mode
  // Transition between alerts,
  switch (currentAlert)
  {
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
      wakeJellyfish();
      break;
  }

  // read magnet sensor input (current prototype for predator nearby--may switch out with time of flight sensor)
  detectPredator();
  if (predatorDetected) {
    warnOtherJellyfish();
  }
}

// FastLED  pattern function

void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS - 1 );
  leds[pos] += CHSV(172, 255, 255); // purple
}

// alert handling logic: capture alert, set up timer start state for each
int TransitionAlertMode(int previousAlertMode)
{
  int alert = myNode.alertReceived();
  switch (alert)
  {
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

void earthquakeSetup()
{
  earthquakeOnRemainingTime = 3000; // set current alert to run for 3 seconds
  floodOnRemainingTime = 0;
  oceanAcidificationOnRemainingTime = 0;
  jellyWarningOnRemainingTime = 0;
  earthquakeOn = false; // start earthquake in off state
}

void floodSetup()
{
  earthquakeOnRemainingTime = 0;
  floodOnRemainingTime = 3000;
  oceanAcidificationOnRemainingTime = 0;
  jellyWarningOnRemainingTime = 0;
  floodOn = false;
}

void oceanAcidificationSetup()
{
  earthquakeOnRemainingTime = 0;
  floodOnRemainingTime = 0;
  oceanAcidificationOnRemainingTime = 5000; // I set this longer so that I can coordinate with a wordy LCD printout for readability
  jellyWarningOnRemainingTime = 0;
  oceanAcidificationOn = false;
}

void jellyWarningSetup()
{
  earthquakeOnRemainingTime = 0;
  floodOnRemainingTime = 0;
  oceanAcidificationOnRemainingTime = 0;
  jellyWarningOnRemainingTime = 3000;
  jellyWarningOn = false;
}

// functions to handle handle alert state and LED behaviors
int earthquakeRender()
{
  if (!earthquakeOn) {
    earthquakeOn = true;
  }

  fill_solid(leds, NUM_LEDS, CRGB::YellowGreen); // set all to the chosen color
  FastLED.show(); // write setting to LEDs
  earthquakeOnRemainingTime -= loopDelta; // count down

  if (earthquakeOnRemainingTime <= 0) {
    earthquakeOn = false;
    return AlertNode::NO_ALERT; // when timer runs out, return default state to main loop
  }

  return AlertNode::EARTHQUAKE;
}

int floodRender()
{
  if (!floodOn) {
    floodOn = true;
  }

  fill_solid(leds, NUM_LEDS, CRGB::Blue); // Set all to the chosen color
  FastLED.show();
  floodOnRemainingTime -= loopDelta;
  Serial.println(floodOnRemainingTime);

  if (floodOnRemainingTime <= 0) {
    floodOn = false;
    return AlertNode::NO_ALERT;
  }
  return AlertNode::FLOOD;
}

int oceanAcidificationRender()
{
  if (!oceanAcidificationOn) {
    oceanAcidificationOn = true;
  }

  fill_solid(leds, NUM_LEDS, CRGB::Maroon); // Set all to the chosen color
  FastLED.show();
  oceanAcidificationOnRemainingTime -= loopDelta;
  Serial.println(oceanAcidificationOnRemainingTime);

  if (oceanAcidificationOnRemainingTime <= 0) {
    oceanAcidificationOn = false;
    return AlertNode::NO_ALERT;
  }
  return AlertNode::OCEAN_ACIDIFICATION;
}

int jellyWarningRender()
{
  if (!jellyWarningOn) {
    jellyWarningOn = true;
  }

  fill_solid(leds, NUM_LEDS, CRGB::Chartreuse); // Set all to the chosen color
  FastLED.show();
  jellyWarningOnRemainingTime -= loopDelta;

  if (jellyWarningOnRemainingTime <= 0) {
    jellyWarningOn = false;
    return AlertNode::NO_ALERT;
  }
  return AlertNode::JELLY_WARNING;
}

//TO DO: convert this to flight time sensor input when part comes in
void detectPredator() {
  int hallEffectSensorValue = digitalRead(hallSensorPin);
  // the sensor turns voltage off (returns 0) when a magnet is present
  predatorDetected = (hallEffectSensorValue == 0);
  // Serial.println(predatorDetected);
}

//TO DO: convert this to new fancy light sensor when part comes in
void senseLight() {
  photoSensorValue = analogRead(photoSensorPin); // read photo sensor input
  //Serial.println(photoSensorValue);
}

void wakeJellyfish() {
  //FastLED.showColor(CRGB::Aqua); // background color
  sinelon(); // this is the current chosen pattern
}

// turns off RGB
void sleepJellyfish() {
  FastLED.clear();
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

// send a special warning to other jellyfish on the node
void warnOtherJellyfish() {
  myNode.sendAlert(AlertNode::JELLY_WARNING);
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
  //  lcd.begin(16, 2); // set number of columns and rows on LCD
  //  lcd.print("Jellyfish "); // print mode to first line
  //  lcd.setCursor(0, 1); // set position to write next line
  //  lcd.print(myNode.alertName(alert) + " DETECTED");
}


