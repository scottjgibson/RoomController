#include <Arduino.h>
#include <Homie.h>
#include <RCSwitch.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// const int PIN_RELAY = 12;
const int PIN_LED = 13;
const int PIN_BUTTON = 0;
const int PIN_RCSWITCH = 16;
const int PIN_MOTION = 14;

bool systemState = false;

unsigned long buttonDownTime = 0;
unsigned long lastMotionTime = 0;
bool motion = false;
byte lastButtonState = 1;
byte buttonPressHandled = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display
RCSwitch mySwitch = RCSwitch();
HomieNode switchNode("switch", "switch");

void setRcSwitch(bool state) {
  if (state)
  {
    mySwitch.send(5575987, 24);
    mySwitch.send(5576131, 24);
    mySwitch.send(5576451, 24);
    lcd.setCursor(7, 1);
    lcd.print("O");
  }
  else
  {
    mySwitch.send(5575996, 24);
    mySwitch.send(5576140, 24);
    mySwitch.send(5576460, 24);
    lcd.setCursor(7, 1);
    lcd.print("-");
  }
}

bool switchOnHandler(HomieRange range, String value) {
  if (value != "true" && value != "false") return false;

  bool on = (value == "true");
  // digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  systemState = on;
  setRcSwitch(systemState);
  switchNode.setProperty("on").send(value);
  Homie.getLogger() << "Switch is " << (on ? "on" : "off") << endl;

  return true;
}

void toggleSystem() {
  systemState = !systemState;
  setRcSwitch(systemState);
  switchNode.setProperty("on").send(systemState ? "false" : "true");
  Homie.getLogger() << "Switch is " << (systemState ? "off" : "on") << endl;
}

void loopHandler() {
  byte buttonState = digitalRead(PIN_BUTTON);
  if ( buttonState != lastButtonState ) {
    if (buttonState == LOW) {
      buttonDownTime     = millis();
      buttonPressHandled = 0;
    }
    else {
      unsigned long dt = millis() - buttonDownTime;
      if ( dt >= 90 && dt <= 900 && buttonPressHandled == 0 ) {
        toggleSystem();
        buttonPressHandled = 1;
      }
    }
    lastButtonState = buttonState;
  }

  if (digitalRead(PIN_MOTION) && !motion)
  {
    lastMotionTime = millis();
    motion = true;
    lcd.setCursor(0, 1);
    lcd.print("Motion");
  //  Homie.setProperty("motion").send("true");
  }
  else if (!digitalRead(PIN_MOTION) && motion)
  {
    motion = false;
    lcd.setCursor(0, 1);
    lcd.print("------");
  //  Homie.setProperty("motion").send("false");
  }

}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_RCSWITCH, OUTPUT);
  pinMode(PIN_MOTION, INPUT);
  mySwitch.enableTransmit(PIN_RCSWITCH);
  mySwitch.setPulseLength(200);
  mySwitch.setRepeatTransmit(15);

  lcd.begin();
  lcd.backlight();
  lcd.home();
  lcd.print("Workshop");
  lcd.setCursor(0, 1);
  lcd.print("----------------");

  Homie_setFirmware("workshop-controller", "1.0.0");
  Homie.setLedPin(PIN_LED, LOW).setResetTrigger(PIN_BUTTON, LOW, 5000);

  switchNode.advertise("on").settable(switchOnHandler);

  Homie.setLoopFunction(loopHandler);
  Homie.setup();
}

void loop() {
  Homie.loop();
}
