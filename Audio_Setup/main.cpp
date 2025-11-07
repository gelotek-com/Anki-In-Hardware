//https://online-voice-recorder.com/
//ffmpeg -i input.mp3 -ac 1 -ar 16000 -acodec pcm_u8 test.wav

#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define SD_CS   10
#define SD_MOSI 7
#define SD_MISO 6
#define SD_SCK  4

const int AUDIO_PIN = 5;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 80000;
const int PWM_RES = 8;

const int SAMPLE_RATE = 16000;

File wavFile;

void skipWavHeader(File &file) {
  file.seek(44);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initializing SD...");

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed!");
    while (true);
  }

  Serial.println("SD OK!");
  wavFile = SD.open("/test.wav");
  if (!wavFile) {
    Serial.println("File open failed!");
    while (true);
  }

  skipWavHeader(wavFile);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(AUDIO_PIN, PWM_CHANNEL);
  Serial.println("Playing audio...");
}

void loop() {
  static uint8_t buffer[512];
  if (!wavFile.available()) {
    Serial.println("Playback done!");
    wavFile.close();
    while (true);
  }

  int bytesRead = wavFile.read(buffer, sizeof(buffer));
  for (int i = 0; i < bytesRead; i++) {
    ledcWrite(PWM_CHANNEL, buffer[i]);
    delayMicroseconds(1000000 / SAMPLE_RATE);
  }
}


