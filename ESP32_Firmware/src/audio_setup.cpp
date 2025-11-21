#include "audio_setup.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <WiFi.h>

const int SD_CS   = 7;
const int SD_MOSI = 6;
const int SD_MISO = 5;
const int SD_SCK  = 4;

const int AUDIO_PIN = 3;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 80000;
const int PWM_RES = 8;
const int SAMPLE_RATE = 16000;

File wavFile;

void setupAudio(){
    Serial.println("Initializing SD...");
    delay(200);
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        Serial.println("Card Mount Failed!");
        while (true);
    }
    Serial.println("SD OK!");

    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
    ledcAttachPin(AUDIO_PIN, PWM_CHANNEL);
}

void skipWavHeader(File &file) {
    file.seek(44);
}

void playAudio(String path) {
    wavFile = SD.open(path.c_str()); 
    if (!wavFile) {
        Serial.print("Audio file not found: ");
        Serial.println(path);
        return;
    }

    skipWavHeader(wavFile);

    static uint8_t buffer[512];

    while (wavFile.available()) {
        int bytesRead = wavFile.read(buffer, sizeof(buffer));
        for (int i = 0; i < bytesRead; i++) {
        ledcWrite(PWM_CHANNEL, buffer[i]);
        delayMicroseconds(1000000 / SAMPLE_RATE);
        }
    }

    wavFile.close();
    Serial.print("Done playing: ");
    Serial.println(path);
}

void playFrontAudio(int button3Pin, int button1Pin, int64_t cardId){
    playAudio( "/" + String(cardId) + "_front.wav");
    while (true) {
      if (digitalRead(button3Pin) == LOW){
        playAudio( "/" + String(cardId) + "_front.wav");
        delay(300);
      }
      if (digitalRead(button1Pin) == LOW) {
        delay(300);
        break;
      }
      delay(50);
      yield();
    }
}

int playBackAudio(int button1Pin, int button2Pin, int button3Pin, int64_t cardId){
    playAudio( "/" + String(cardId) + "_back.wav");

    while (true) {
      if (digitalRead(button3Pin) == LOW){
        playAudio( "/" + String(cardId) + "_back.wav");
        delay(300);
      }
      if (digitalRead(button1Pin) == LOW) {
        delay(300);
        return 1;
      }
      if (digitalRead(button2Pin) == LOW) {
        delay(300);
        return 4;
      }
      delay(50);
      yield();
    }
}
void clearRootFiles() {
    File root = SD.open("/");
    File file = root.openNextFile();

    while (file) {
        if (!file.isDirectory()) {
            String path = "/";
            path = path + file.name();
            Serial.printf("Removing: %s\n", path.c_str());
            SD.remove(path);
        }
        file = root.openNextFile();
    }
}