
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

const String myNodeName = "Jellyfish"; // name node for use with AlertNodeLib
const boolean SENDING = true; // settings for AlertLib
const boolean RECEIVING = true; // setting for AlertLib
boolean myDebugging = true; // settings for AlertLib
//const int debugPin = LED_BUILTIN; // set built in LED for debugging - commented b/c I'm currently using pin 13 in my build
const int hallSensorPin = 7; // magnet sensor is attached here
const int photoSensorPin = A0; // connect sensor to analog pin
//const int redPin = 11; - currently testing LED strip instead
//const int greenPin = 10;
//const int bluePin = 9;
// settings for FastLED with APA102 DotStar strip
const int DATA_PIN = 5;
const int CLK_PIN = 13;
#define LED_TYPE APA102
#define COLOR_ORDER BGR
#define NUM_LEDS 20
const int BRIGHTNESS = 96;
const int FRAMES_PER_SECOND = 120;
CRGB leds[NUM_LEDS];

Servo jellyMover; // servo to control jellyfish
Adafruit_LiquidCrystal lcd(0); // initialize LCD
AlertNode myNode; // connect to XBee; pin 2 TX and pin 3 for RX, 9600 baud

int position; // servo position
int predatorDetected = 0; // state of predators
int photoSensorValue = 0; // amount of light

// TODO: add state for LED, add timer

unsigned long previousMillis = 0;
unsigned long interval = 5000; // how long to run LED pattern for

void setup() {
  delay(3000); // 3 second recommended delay for recovery for FastLED
  Serial.begin(9600);
  myNode.begin(myNodeName); // start node
  Serial.print("\n\n*** Starting AlertNodeBasic demo: ");
  Serial.println(myNodeName);
  myNode.setDebug(true);

  jellyMover.attach(4); // set jellyfish on pin 4
  pinMode(hallSensorPin, INPUT);  // set sensor pin as input
  //  pinMode(redPin, OUTPUT);
  //  pinMode(greenPin, OUTPUT);
  //  pinMode(bluePin, OUTPUT);
  // start LED in off state - currently testing LED strip instead
  //analogWrite(greenPin, 255);
  //analogWrite(redPin, 255);
  //analogWrite(bluePin, 255);

  // setup for FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // set brightness control
  FastLED.setBrightness(BRIGHTNESS);

  sleepJellyfish();
}

uint8_t gHue = 0; // rotating "base color" used by many FastLED patterns

void loop() {
  // put your main code here, to run repeatedly:
  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);
  unsigned long currentMillis = millis(); // timer
  //if ((unsigned long)(currentMillis - previousMillis) >= interval) {
  //    Serial.println(currentMillis);
  //  }
  //check alerts
  int alert = myNode.alertReceived();
  if (alert != AlertNode::NO_ALERT) {
    Serial.println("*** Alert received: ");
    Serial.print(alert);
    Serial.print(" ");
    Serial.println(myNode.alertName(alert));
    if (alert == AlertNode::EARTHQUAKE) {
      //earthquake
      FastLED.showColor(CRGB::Yellow);
      delay(5000);
    }
    else if (alert == AlertNode::FLOOD) {
      // do flood thing - e.g. move a lot more
      FastLED.showColor(CRGB::Green);
      delay(5000);
    }
    else if (alert == AlertNode::OCEAN_ACIDIFICATION) {
      // do red thing
      FastLED.showColor(CRGB::Red);
      delay(5000);
    }
  }

  // jellyfish creates light when it's dark
  senseLight();
  if (photoSensorValue < 200) {
    wakeJellyfish();
  }
  else if (photoSensorValue > 200) {
    sleepJellyfish(); //if it's bright, turn off LEDs
  }

  // read magnet sensor input (current prototype for predator nearby--may switch out with time of flight sensor)
  detectPredator();
  if (predatorDetected) {
    //swayJellyfish();
  }
}

// FastLED example pattern functions for testing
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS - 1 );
  leds[pos] += CHSV(160, 255, 255); // purple
}

void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for ( int i = 0; i < 8; i++) {
    leds[beatsin16( i + 7, 0, NUM_LEDS - 1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

//TODO: detectAlert(), colorJellyfish(), writeLCD(), sendAlert()

//void colorJellyfish() {
//  // takes a color
//  // map to a definition
//  // rgb value
//  // red / orange
//  // purple / blue
//  // bright chartruse/green
//}

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

// TO DO: update this to set a default awake color transition (e.g. bright green, then fade to aqua)
void wakeJellyfish() {
  FastLED.showColor(CRGB::Aqua); // background color
  sinelon();
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

