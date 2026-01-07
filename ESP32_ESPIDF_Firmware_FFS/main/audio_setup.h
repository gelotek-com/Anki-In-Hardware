#pragma once

#include <cstdint>
#include <string>

// Initialization
void setupAudio();

// Playback
void playAudio(const std::string &path);
void playFrontAudio(int button3Pin, int button1Pin, int64_t cardId);
int  playBackAudio(int button1Pin, int button2Pin, int button3Pin, int64_t cardId);

// File management
void clearRootFiles();
void clearALLRootFiles();
bool checkAudioFiles(int64_t cardId);
