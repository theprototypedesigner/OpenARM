/* This is all the blueprint specific code for all of our
 *  iOS switch control example code. We have moved it here to make
 *  the main source code more legible.
 */
 
#include <EEPROMex.h>
#include <SPI.h>
#include <CapacitiveSensor.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h"

//Pin numbers for switches, LEDs and to read battery voltage
#define SWITCH_01 A0
#define SWITCH_02 A1
#define SWITCH_03 A2
#define SWITCH_04 A3
#define GREEN_LED 13
#define RED_LED 11
#define VBATPIN A9

//Actions
#define PRESS_FIRST 1
#define PRESS_SECOND 2
#define PRESS_THIRD 4
#define PRESS_FOURTH 8
#define PRESS_FIRST_TOUCH 16
#define PRESS_SECOND_TOUCH 32

#define WAIT 10

const int feedbackPins[] = { -1, A5, A4};
unsigned long treshold_1, treshold_2, batTime, blinkTime, feedbackTime;
uint8_t valTouch1 = 0, valTouch2 = 0, count, state = 1, feedbackPin, feedbackState = LOW, prevFeedbackState;
int capacitiveMemAddress1, capacitiveMemAddress2;

CapacitiveSensor capacitive_01 = CapacitiveSensor(10, 2);
CapacitiveSensor capacitive_02 = CapacitiveSensor(10, 3);

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);



//Debug output routines
#if (MY_DEBUG)
  #define MESSAGE(m) Serial.println(m);
  #define FATAL(m) {MESSAGE(m); while (1);}
#else
  #define MESSAGE(m) {}
  #define FATAL(m) while (1);
#endif

void initializeBluefruit (void) {
  //Initialize Bluetooth
  if ( !ble.begin(MY_DEBUG))
  {
    FATAL(F("NO BLE?"));
  }
  //Rename device
  if (! ble.sendCommandCheckOK(F( "AT+GAPDEVNAME=iOS Switch Access" )) ) {
    FATAL(F("err:rename fail"));
  }
  //Enable HID keyboard
  if(!ble.sendCommandCheckOK(F( "AT+BleHIDEn=On" ))) {
    FATAL(F("err:enable Kb"));
  }
  //Add or remove service requires a reset
  if (! ble.reset() ) {
    FATAL(F("err:SW reset"));
  }
}

void blink() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(RED_LED, LOW);
    delay(100);
    digitalWrite(RED_LED, HIGH);
    delay(100);
  }
}

