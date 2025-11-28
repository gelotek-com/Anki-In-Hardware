#pragma once
#include <Arduino.h>

extern const int button1Pin; //yellow button
extern const int button2Pin; //red button
extern const int button3Pin; //red button

void setupButtons();
void waitForButton(int pin, int led, const char* message);
int waitForStudy(int pin, int pin2, int led, const char* message);