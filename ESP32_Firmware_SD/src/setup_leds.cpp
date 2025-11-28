#include "setup_leds.h"

const int ledRed = 20;
const int ledGreen = 21;
const int ledBlue = 9;

void setupLeds(){
    pinMode(ledRed, OUTPUT);
    pinMode(ledGreen, OUTPUT);
    pinMode(ledBlue, OUTPUT);
    digitalWrite(ledRed, LOW);
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledBlue, LOW);
}

