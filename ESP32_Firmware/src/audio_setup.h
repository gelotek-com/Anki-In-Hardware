#pragma once
#include <Arduino.h>
#include "FS.h"

//sd
extern const int SD_CS;
extern const int SD_MOSI;
extern const int SD_MISO;
extern const int SD_SCK;
//amps
extern const int AUDIO_PIN;
extern const int PWM_CHANNEL;
extern const int PWM_FREQ;
extern const int PWM_RES;
extern const int SAMPLE_RATE;

extern File wavFile;

void setupAudio();
void skipWavHeader(File &file);
void playAudio(String path);
void playFrontAudio(int button3Pin, int button1Pin, int64_t cardId);
int playBackAudio(int button1Pin, int button2Pin, int button3Pin, int64_t cardId);