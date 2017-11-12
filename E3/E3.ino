/* 539 E3: External Stimuli
   joeyhoy@uw.edu
   Uses AlertNodeLib by ahdavidson
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
const int photoSensorPin = A0; // connect sensor to analog pin
int photoSensorValue = 0;

// state variables
boolean alarmState = true; // alarm state default to on
String timeOfDay;

AlertNode myNode; // connect to XBee; pin 2 TX and pin 3 for RX, 9600 baud
Button alarmToggle(btnPin); // create button to turn alarm on and off
Adafruit_LiquidCrystal lcd(0); // initialize LCD

void setup() {
  // put your setup code here, to run once:

  // debugging outputs
  Serial.begin(9600);
  pinMode(debugPin, OUTPUT);

  myNode.setDebugOn();
  myNode.begin(myNodeName); // start node
}

void loop() {
  // put your main code here, to run repeatedly:

  setAlarm();

  getTimeOfDay();

  if (alarmState) {
    int alert = myNode.alertReceived();
    if (alert != AlertNode::NO_ALERT) {
      // process alert
      if (alert == AlertNode::FIRE) {
        logAlert(myNodeName, alert);
      }
      else if (alert == AlertNode::FLOOD && timeOfDay == "night") {
        logAlert(myNodeName, alert);
        soundAlarm(ringPin);
      }
      else logAlert(myNodeName, alert);
    }
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

void soundAlarm (int ringPin) {
  tone(ringPin, 4186, 500);
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

  lcd.begin(16, 2);
  // print a message to the LCD
  lcd.print(myNode.alertName(alert) + " in the " + timeOfDay);
}
