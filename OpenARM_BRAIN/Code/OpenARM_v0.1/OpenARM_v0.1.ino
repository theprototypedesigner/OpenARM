/*
   OpenARM is an open source project designed for people
   with upper body disabilities, who may struggle with using
   mobile devices such as smartphones, tablets as well as desktop devices.
   The project aim is to give people a more affordable and customizable version
   of the controllers already existing in the market.

   The following code is based on the work of Chris Young:
   https://learn.adafruit.com/ios-switch-control-using-ble?view=all#overview
*/


#define MY_DEBUG 0 //1 to run the sketch in debug mode
#include "Initializations.h"


void setup() {
#if(MY_DEBUG)
  while (! Serial) {}; delay (500);
  Serial.begin(9600);
  MESSAGE(F("Debug ON"));
#endif

  pinMode(SWITCH_01, INPUT_PULLUP);
  pinMode(SWITCH_02, INPUT_PULLUP);
  pinMode(SWITCH_03, INPUT_PULLUP);
  pinMode(SWITCH_04, INPUT_PULLUP);
  pinMode(feedbackPins[1],OUTPUT);
  pinMode(feedbackPins[2],OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  digitalWrite(RED_LED, HIGH);

  //start bluetooth communication
  initializeBluefruit();

  //assign the EEPROM address to retrieve the threshold values for touch switches
  capacitiveMemAddress1 = EEPROM.getAddress(sizeof(long));
  delay(WAIT);
  capacitiveMemAddress2 = EEPROM.getAddress(sizeof(long));
  delay(WAIT);

  //retrive the treshold values for touch switches from EEPROM
  treshold_1 = EEPROM.readLong(capacitiveMemAddress1);
  delay(WAIT);
  treshold_2 = EEPROM.readLong(capacitiveMemAddress2);
  delay(WAIT);

  //check if any touch switche is connected
  checkTouchSwitches();
  
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
}

void loop() {
  batteryVoltage(); //check the battery voltage
  uint8_t i = readSwitches(); //check if any of the switches has been pressed
  switch (i) { //send a letter through the bluetooth according to the button pressed
    case PRESS_FIRST:        pressKeyCode('a'); break;
    case PRESS_SECOND:       pressKeyCode('b'); break;
    case PRESS_THIRD:        pressKeyCode('c'); break;
    case PRESS_FOURTH:       pressKeyCode('d'); break;
    case PRESS_FIRST_TOUCH:  pressKeyCode('e'); break;
    case PRESS_SECOND_TOUCH: pressKeyCode('f'); break;
  }
  feedback(LOW);
}

void touchCalibration() {
  MESSAGE(F("Don't touch anything!"));
  blink();

  unsigned long sum = 0, i = 0, val = 0, low_1 = 0, low_2 = 0, high_1 = 0, high_2 = 0;
  delay(1000); //delay to allow the millis to be higher than what required for the first while
  unsigned long start = millis();
  while (millis() - 3000 < start) {
    val = capacitive_01.capacitiveSensor(30);
    sum += val;
    i++;
    delay(10);
  }
  low_1 = sum / i;

  sum = 0; i = 0;
  start = millis();
  while (millis() - 3000 < start) {
    val = capacitive_02.capacitiveSensor(30);
    sum += val;
    i++;
    delay(10);
  }
  low_2 = sum / i;

  MESSAGE(F("Now touch the left button"));
  blink();
  sum = 0; i = 0;
  start = millis();
  while (millis() - 5000 < start) {
    val = capacitive_01.capacitiveSensor(30);
    sum += val;
    i++;
    delay(10);
  }
  high_1 = sum / i;

  MESSAGE(F("Now touch the right button"));
  blink();
  sum = 0; i = 0;
  start = millis();
  while (millis() - 5000 < start) {
    val = capacitive_02.capacitiveSensor(30);
    sum += val;
    i++;
    delay(10);
  }
  high_2 = sum / i;

  MESSAGE(F("You can remove your hand from the controller"));
  blink();

  //calculating the thresholds and storing into the EEPROM
  treshold_1 = (low_1 + high_1) / 2;
  EEPROM.updateLong(capacitiveMemAddress1, treshold_1);
  delay(WAIT);
  treshold_2 = (low_2 + high_2) / 2;
  EEPROM.updateLong(capacitiveMemAddress2, treshold_2);
  delay(WAIT);

  MESSAGE(treshold_1);
  MESSAGE(treshold_2);
  MESSAGE(F("DONE!"));
  digitalWrite(13, LOW);
}

uint8_t readSwitches(void) {
  return (~(digitalRead(SWITCH_01) * PRESS_FIRST
            + digitalRead(SWITCH_02) * PRESS_SECOND
            + digitalRead (SWITCH_03) * PRESS_THIRD
            + digitalRead (SWITCH_04) * PRESS_FOURTH
            + readTouchSwitch(valTouch1) * PRESS_FIRST_TOUCH
            + readTouchSwitch(valTouch2) * PRESS_SECOND_TOUCH)
         ) & (PRESS_FIRST + PRESS_SECOND + PRESS_THIRD + PRESS_FOURTH + PRESS_FIRST_TOUCH + PRESS_SECOND_TOUCH);
}

uint8_t readTouchSwitch(uint8_t _switch) {
  long val; unsigned long treshold;
  if (_switch == 0) {
    return 1;
  } else if (_switch == 1) {
    val = capacitive_01.capacitiveSensor(30);
    treshold = treshold_1;
  } else {
    val = capacitive_02.capacitiveSensor(30);
    treshold = treshold_2;
  }
  if (val > treshold) {
    feedbackPin = feedbackPins[_switch];
    digitalWrite(RED_LED, HIGH);
    return 0;
  } else {
    digitalWrite(RED_LED, LOW);
    return 1;
  }
}

//Translate character to keyboard keycode and transmit
void pressKeyCode (uint8_t c) {
  ble.print(F("AT+BLEKEYBOARDCODE=00-00-"));
  uint8_t Code = c - 'a' + 4;
  if (Code < 0x10)ble.print("0");
  ble.print(Code, HEX);
  ble.println(F("-00-00-00-00"));
  MESSAGE(F("Pressed."));
  delay(100);//de-bounce
  feedback(HIGH);
  while (readSwitches()) { //wait for button to be released
    feedback(LOW);
  };
  digitalWrite(GREEN_LED, HIGH);
  ble.println(F("AT+BLEKEYBOARDCODE=00-00"));
  MESSAGE(F("Released"));
}

void batteryVoltage() {
  float batVoltage = analogRead(VBATPIN);
  batVoltage *= 2;    // the reading is divided by 2, so multiply back
  batVoltage *= 3.3;  // Multiply by 3.3V, the reference voltage of the board
  batVoltage /= 1024; // convert to voltage

  //if battery voltage is under 3.4 then every 10 seconds the green led blink 3 times
  if (batVoltage < 3.4) {
    if (millis() - 10000 > batTime) {
      MESSAGE(F("Low battery, please recharge"));
      if (millis() - 200 > blinkTime) {
        count++;
        blinkTime = millis();
        state = !state;
        digitalWrite(GREEN_LED, state);
      }
      if (count > 5) {
        count = 0;
        batTime = millis();
      }
    }
  }
}

void checkTouchSwitches() {
  bool calibDone = false;
  unsigned long val = 0, x = 0;

  if (capacitive_01.capacitiveSensor(30) != -2) {
    valTouch1 = 1;
    MESSAGE(F("Touch Switch 1 detected"));
    //treshold_1 = EEPROM.readLong(capacitiveMemAddress1);
    //delay(WAIT);
    for (int i = 0; i < 10; i++) {
      x = capacitive_01.capacitiveSensor(30);
      if (x > val)  val = x;
      delay(10);
    }
    if (val > (treshold_1 / 10)) {
      calibDone = true;
      touchCalibration();
    }
  }
  if (capacitive_02.capacitiveSensor(30) != -2) {
    valTouch2 = 2;
    MESSAGE(F("Touch Switch 2 detected"));
    //treshold_2 = EEPROM.readLong(capacitiveMemAddress2);
    //delay(WAIT);
    if (calibDone == false) {
      for (int i = 0; i < 10; i++) {
        x = capacitive_02.capacitiveSensor(30);
        if (x > val) val = x;
        delay(10);
      }
      if (val > (treshold_2 / 10)) {
        touchCalibration();
      }
    }
  }
}

void feedback(uint8_t fState) {
  if (fState == HIGH) {
    digitalWrite(feedbackPin, fState);
    feedbackTime = millis();
  } else {
    if (millis() - 100 > feedbackTime) {
      digitalWrite(feedbackPin, fState);
    }
  }
}
