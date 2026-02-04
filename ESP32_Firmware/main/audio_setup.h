#pragma once

#include <cstdint>
#include <string>


// Initialization
void setupAudio();

// Playback
void playAudio(const std::string &path, const std::string &storage);
void playFrontAudio(int button3Pin, int button1Pin, int64_t cardId);
int  playBackAudio(int button1Pin, int button2Pin, int button3Pin, int64_t cardId);

// File management
bool checkAudioFiles_ext(int64_t cardId);
void clearIntFiles();
void clearExtFiles();
