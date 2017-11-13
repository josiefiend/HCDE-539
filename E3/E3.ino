/* 539 E3: External Stimuli
   joeyhoy@uw.edu
   Uses AlertNodeLib and Button library by ahdavidson, LCD library by Adafruit
*/

// include libraries for AlertNode, XBee, LCD, and Button

#include <SoftwareSerial.h>
#include <Adafruit_LiquidCrystal.h>
#include <AlertNodeLib.h>
#include <Button.h>

const String myNodeName = "Node Awesome"; // name node for use with AlertNodeLib
const int debugPin = LED_BUILTIN; // set built in LED for debugging
const int btnPin = 7; // set button pin
const int ringPin = 8; // set alarm pin
const int flashPin = 12; // set LED flash pin
const int photoSensorPin = A0; // connect sensor to analog pin
int photoSensorValue = 0;

// state variables
boolean alarmState = true; // alarm state default to on
String timeOfDay;
String alertFriendly; // TO DO: translate alert values into something more human-readable

AlertNode myNode; // connect to XBee; pin 2 TX and pin 3 for RX, 9600 baud
Button alarmToggle(btnPin); // create button to turn alarm on and off
Adafruit_LiquidCrystal lcd(0); // initialize LCD

void setup() {
  // put your setup code here, to run once:

  // debugging outputs
  Serial.begin(9600);
  pinMode(debugPin, OUTPUT);
  digitalWrite(debugPin, HIGH);
  delay(500);
  digitalWrite(debugPin, LOW);

  pinMode(flashPin, OUTPUT); // set LED pin as output
  myNode.setDebugOn();
  myNode.begin(myNodeName); // start node
}

void loop() {
  // put your main code here, to run repeatedly:

  setAlarm();

  getTimeOfDay();

  if (alarmState) {
    digitalWrite(debugPin, HIGH);
    int alert = myNode.alertReceived();
    if (alert != AlertNode::NO_ALERT) {
      // when fire alert is received during the day: display text alert, sound alarm twice
      if (alert == AlertNode::FIRE && timeOfDay == "day") {
        logAlert(myNodeName, alert);
        soundAlarm(2);
      }
      // when fire alert is received at night: display text alert, sound two beep alarm and also flash light
      else if (alert == AlertNode::FIRE && timeOfDay == "night") {
        logAlert(myNodeName, alert);
        soundAlarm(2);
        flashLED(2);
      }
      // when flood alert is received during the day: display text alert, sound alarm and also flash light
      else if (alert == AlertNode::FLOOD && timeOfDay == "day") {
        logAlert(myNodeName, alert);
        soundAlarm(1);
      }
      // when flood alert is received at night: display text alert, sound alarm and also flash light
      else if (alert == AlertNode::FLOOD && timeOfDay == "night") {
        logAlert(myNodeName, alert);
        soundAlarm(1);
        flashLED(1);
      }
      // when burglar alert is received during the day: display text alert, sound three beep alarm
      else if (alert == AlertNode::BURGLARY && timeOfDay == "day") {
        logAlert(myNodeName, alert);
        soundAlarm(3);
      }
      // when burglar alert is received at night: display text alert, sound three beep alarm
      else if (alert == AlertNode::BURGLARY && timeOfDay == "night") {
        logAlert(myNodeName, alert);
        soundAlarm(4);
      }
      // when apocalpyse alert is received: display text alert, sound 10 beep alarm and flash light 10 times
      else if (alert == AlertNode::APOCALYPSE) {
        logAlert(myNodeName, alert);
        soundAlarm(10);
        flashLED(10);
      }
      else logAlert(myNodeName, alert);
    }
  }
  else if (!alarmState) {
    int alert = myNode.alertReceived(); // capture alert; this is so when turned back on it's showing the most current alert
    digitalWrite(debugPin, LOW); // turn of LED built in
    lcd.setCursor(0, 1);
    lcd.print (" ");
  }
}

void setAlarm () {
  if (alarmToggle.checkButtonAction() == Button::PRESSED) {
    alarmState = !alarmState;
    Serial.print("+++ Setting system status ");
    Serial.println(alarmState ? "ARMED" : "DISARMED");
    lcd.begin(16, 1);
    lcd.print(alarmState ? "ARMED" : "DISARMED");
    myNode.setDebug(alarmState);
  }
}

void getTimeOfDay() {
  photoSensorValue = analogRead(photoSensorPin); // read photo sensor input
  if (photoSensorValue > 200) {
    timeOfDay = "day";
  }
  else timeOfDay = "night";
}

void soundAlarm (int numRings) {
  for (int x = 0; x < numRings; x++) {
    tone(ringPin, 4186, 500);
    delay(1000);
  }
}

void flashLED (int numRings) {
  for (int x = 0; x < numRings; x++) {
    digitalWrite(flashPin, HIGH);
    delay(100);
    digitalWrite(flashPin, LOW);
    delay(100);
  }
}

void logAlert (String myName, int alert) {

  // if alarm is on
  if (alarmState) {
    Serial.print("*** alert received at ");
    float sec = millis() / 1000.0;
    Serial.println(sec);
    Serial.print("***   node = ^");
    Serial.print(myName);
    Serial.println("^");
    Serial.print("***   alert = ");
    Serial.print(alert);
    Serial.print(": ");
    Serial.println(myNode.alertName(alert));
  }

  lcd.begin(16, 2); // set number of columns and rows on LCD
  lcd.print(timeOfDay + " mode"); // print mode to first line
  lcd.setCursor(0, 1); // set position to write next line
  lcd.print(myNode.alertName(alert) + " DETECTED");
}
