#pragma once

#include <cstdint>

// Button GPIOs
extern const int button1Pin;
extern const int button2Pin;
extern const int button3Pin;

// API
void setupButtons();
void waitForButton(int ledGreen, int ledBlue, int ledRed, const char *message);
int  waitForStudy(int pin, int pin2, int led, const char *message);
