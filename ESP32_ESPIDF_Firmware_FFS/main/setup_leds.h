#pragma once

#include "driver/gpio.h"

extern const gpio_num_t ledRed;
extern const gpio_num_t ledGreen;
extern const gpio_num_t ledBlue;

// API
void setupLeds();
