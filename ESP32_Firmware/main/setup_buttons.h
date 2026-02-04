#pragma once

#include <cstdint>

extern const int button1Pin;
extern const int button2Pin;
extern const int button3Pin;

void setupButtons();
void waitForButton(const char *message);
