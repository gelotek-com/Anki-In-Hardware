#include "audio_setup.h"
#include <SPIFFS.h>
#include <WiFi.h>

const int AUDIO_PIN = 3;
const int PWM_CHANNEL = 0;
const int PWM_FREQ = 80000;
const int PWM_RES = 8;
const int SAMPLE_RATE = 16000;

File wavFile;

void setupAudio() {
  Serial.println("Initializing SPIFFS...");
  delay(200);

  if (!SPIFFS.begin(true)) {   // true = auto-format if filesystem is empty
      Serial.println("SPIFFS Mount Failed!");
      while (true);
  }

  Serial.println("SPIFFS OK!");

  // setup PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(AUDIO_PIN, PWM_CHANNEL);
}

void skipWavHeader(File &file) {
  file.seek(44);
}

void clearRootFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file) {
    String name = file.name();
    //name == "clear.wav" || name == "download.wav"|| name == "downloading.wav" || name == "end.wav"|| name == "start.wav" || name == "response.wav"
    if (name == "clear.wav" || name == "download.wav"|| name == "downloading.wav" || name == "end.wav"|| name == "start.wav"|| name == "response.wav" ) {
      Serial.printf("Keeping: %s\n", name.c_str());
    } else {
      String path = "/" + name;
      Serial.printf("Removing: %s\n", path.c_str());
      SPIFFS.remove(path);
    }
    file = root.openNextFile();
  }

  Serial.println("Finished clearing SPIFFS");
}

void playAudio(String path) {
  wavFile = SPIFFS.open(path, "r");
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

bool checkAudioFiles(int64_t cardId){
  String front = "/" + String(cardId) + "_front.wav";
  String back = "/" + String(cardId) + "_back.wav";

  if (SPIFFS.exists(front)) {
    if (SPIFFS.exists(back)) {
      Serial.println("Audio files exist");
      return true;
    }
  }
  Serial.println("Audio files do not exist, this card cannot be studied");
  return false;
}