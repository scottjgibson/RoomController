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


unsigned long buttonDownTime = 0;
unsigned long lastMotionTime = 0;
unsigned long offTime = 0;
bool motion = false;
byte lastButtonState = 1;
byte buttonPressHandled = 0;

#define STATE_OFF_AUTO           0
#define STATE_ON_AUTO            1
#define STATE_ON_HOLD            2
#define STATE_OFF_MOTION_DISABLE 3
int systemState = STATE_OFF_AUTO;

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display
RCSwitch mySwitch = RCSwitch();
HomieNode switchNode("switch", "switch");

void setRcSwitch(bool state) {
  if (state)
  {
    mySwitch.send(5575987, 24);
    mySwitch.send(5576131, 24);
    mySwitch.send(5576451, 24);
    lcd.setCursor(13, 1);
    lcd.print("O");
  }
  else
  {
    mySwitch.send(5575996, 24);
    mySwitch.send(5576140, 24);
    mySwitch.send(5576460, 24);
    lcd.setCursor(13, 1);
    lcd.print(" ");
  }
}

bool switchOnHandler(HomieRange range, String value) {
  if (value != "true" && value != "false") return false;

  if (value == "true") {
    systemState = STATE_ON_AUTO;
    lastMotionTime = millis();
    lcd.setCursor(0, 0);
    lcd.print("ON - LIGHTS  ");
   Homie.getLogger() << "1 - systemState:" << String(systemState) <<endl;
    setRcSwitch(true);
    switchNode.setProperty("on").send("true");
    Homie.getLogger() << "MQTT - Switch is ON - LIGHTS" << endl;
  }
  else{
    systemState = STATE_OFF_AUTO;
    lcd.setCursor(0, 0);
    lcd.print("OFF - AUTO ");
   Homie.getLogger() << "2 - systemState:" << String(systemState) <<endl;
    setRcSwitch(false);
    switchNode.setProperty("on").send("false");
    Homie.getLogger() << "MQTT - Switch is OFF - AUTO" << endl;
  }
  return true;
}

void regularButtonPress() {
  bool on = false;
  if ((systemState == STATE_ON_AUTO) || (systemState == STATE_ON_HOLD)) {
    systemState = STATE_OFF_MOTION_DISABLE;
    Homie.getLogger() << "3 - systemState:" << String(systemState) <<endl;
    offTime = millis();
    lcd.setCursor(0, 0);
    lcd.print("OFF - DELAY ");
    setRcSwitch(false);
    switchNode.setProperty("on").send("false");
    switchNode.setProperty("all_on").send("false");
    Homie.getLogger() << "Regular Button Press: Switch is off - motion diable"<< endl;
  }
  else {
    systemState = STATE_ON_AUTO;
    lastMotionTime = millis();
    Homie.getLogger() << "4 - systemState:" << String(systemState) <<endl;
    lcd.setCursor(0, 0);
    lcd.print("ON - LIGHTS  ");
    setRcSwitch(true);
    switchNode.setProperty("on").send("true");
    Homie.getLogger() << "Regular Button Press: Switch is on - auto" << endl;
  }
}

void longButtonPress() {
  if (systemState == STATE_ON_AUTO) {
    systemState = STATE_ON_HOLD;
    Homie.getLogger() << "5 - systemState:" << String(systemState) <<endl;
    lcd.setCursor(0, 0);
    lcd.print("ON - ALL   ");
    setRcSwitch(true);
    switchNode.setProperty("on").send("true");
    switchNode.setProperty("all_on").send("true");
    Homie.getLogger() << "Long Button Press: Switch is ON - ALL " << endl;
    lcd.setCursor(0, 1);
    lcd.print("      ");
  }
}

void loopHandler() {
  // lcd.setCursor(15, 0);
  // lcd.print(systemState);
  // Homie.getLogger() << "LOOP - systemState:" << String(systemState) <<endl;

  byte buttonState = digitalRead(PIN_BUTTON);
  if ( buttonState != lastButtonState ) {
    if (buttonState == LOW) {
      buttonDownTime     = millis();
      buttonPressHandled = 0;
    }
    else {
      unsigned long dt = millis() - buttonDownTime;
      if ( dt >= 90 && dt <= 900 && buttonPressHandled == 0 ) {
        regularButtonPress();
        buttonPressHandled = 1;
      }
      if ( dt > 900 && dt <= 3000 && buttonPressHandled == 0 ) {
        longButtonPress();
        buttonPressHandled = 1;
      }
    }
    lastButtonState = buttonState;
  }

  if (digitalRead(PIN_MOTION) && !motion) {
    lastMotionTime = millis();
    motion = true;
    lcd.setCursor(15, 1);
    lcd.print("M");
    if (systemState == STATE_OFF_AUTO) {
        systemState = STATE_ON_AUTO;
        Homie.getLogger() << "6 - systemState:" << String(systemState) <<endl;
        lcd.setCursor(0, 0);
        lcd.print("ON - LIGHTS  ");
        setRcSwitch(true);
        switchNode.setProperty("on").send("true");
        Homie.getLogger() << "Motion - Switch is on - auto" << endl;
    }
  }
  else if (digitalRead(PIN_MOTION)) {
    lastMotionTime = millis();
  }
  else if (!digitalRead(PIN_MOTION) && motion) {
    motion = false;
    lcd.setCursor(15, 1);
    lcd.print(" ");
  }

  if (Homie.isConfigured()) {
   if (Homie.isConnected()) {
     lcd.setCursor(14, 1);
     lcd.print("C");
   } else {
     lcd.setCursor(14, 1);
     lcd.print("D");
   }
 }

  if (systemState == STATE_ON_AUTO) {
    unsigned int elapsedTime = (millis() - lastMotionTime)/1000;
    int remainingTime = 1800 - elapsedTime;
    int remainingMin = remainingTime / 60;
    int remainingSec = remainingTime % 60;
    if (remainingMin > 9) {
      lcd.setCursor(0, 1);
      lcd.print(remainingMin);
    }
    else {
      lcd.setCursor(0, 0);
      lcd.print("0");
      lcd.setCursor(0, 1);
      lcd.print(remainingMin);
    }
    lcd.setCursor(2, 1);
    lcd.print(":");
    if (remainingSec > 9){
      lcd.setCursor(3, 1);
      lcd.print(remainingSec);
    }
    else {
      lcd.setCursor(3, 1);
      lcd.print("0");
      lcd.setCursor(4, 1);
      lcd.print(remainingSec);
    }
    if (remainingTime < 0) {
      Homie.getLogger() << "EXPIRED" << endl;
      systemState = STATE_OFF_AUTO;
      setRcSwitch(false);
      Homie.getLogger() << "8 - systemState:" << String(systemState) <<endl;
      lcd.setCursor(0, 0);
      lcd.print("OFF - AUTO  ");
      Homie.getLogger() << "OFF - AUTO" << endl;
      lcd.setCursor(0, 1);
      lcd.print("          ");
    }
  }
  if (systemState == STATE_OFF_MOTION_DISABLE) {
    unsigned int elapsedTime = (millis() - offTime)/1000;
    lcd.setCursor(0, 1);
    lcd.print("00:");
    if (30 - elapsedTime > 9) {
      lcd.setCursor(3, 1);
      lcd.print(30 - elapsedTime);
    }
    else {
      lcd.setCursor(3, 1);
      lcd.print("0");
      lcd.setCursor(4, 1);
      lcd.print(30 - elapsedTime);
    }
    if (elapsedTime >= 30) {
      Homie.getLogger() << "EXPIRED" << endl;
      systemState = STATE_OFF_AUTO;
      setRcSwitch(false);
      Homie.getLogger() << "7 - systemState:" << String(systemState) <<endl;
      lcd.setCursor(0, 0);
      lcd.print("OFF - AUTO  ");
      Homie.getLogger() << "OFF - AUTO" << endl;
      lcd.setCursor(0, 1);
      lcd.print("          ");
    }
  }

}

void setup() {
  Serial.begin(115200);
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
  systemState = STATE_OFF_AUTO;
  setRcSwitch(false);
  lcd.setCursor(0, 0);
  lcd.print("OFF - AUTO ");

  Homie_setFirmware("workshop-controller", "1.0.0");
  Homie.setLedPin(PIN_LED, LOW).setResetTrigger(PIN_BUTTON, LOW, 10000);

  switchNode.advertise("on").settable(switchOnHandler);
  switchNode.advertise("all_on");

  Homie.setLoopFunction(loopHandler);
  Homie.setup();
}

void loop() {
  Homie.loop();
}
