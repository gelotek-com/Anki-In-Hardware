#include "http_setup.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <WiFi.h>

const char* server = "http://";

String HTTPboot(){
    String cardData;

    HTTPClient http;
    http.begin(String(server) + "/boot");
    http.addHeader("Content-Type", "application/json");
    int code = http.POST("{\"device\":\"esp32\"}");
    if (code > 0) {
      cardData = http.getString();
      Serial.printf("Received cards [%d]: %s\n", code, cardData.c_str());
    } else {
      Serial.printf("Error on /boot: %s\n", http.errorToString(code).c_str());
    }
    http.end();
    return cardData;
}


void HTTPout(int ledRed, String jsonOut) {
    HTTPClient http;
    http.begin(String(server) + "/ease");
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(jsonOut);
    String resp = http.getString();
    Serial.printf("Ease response [%d]: %s\n", code, resp.c_str());
    if (code <= 0) {
      digitalWrite(ledRed, HIGH);
    }
    http.end();
}


bool downloadFile(const String& filename) {
  String path = "/" + filename;

  String url = String(server) + "/audio/" + filename;
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing: " + path);
      http.end();
      return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buff[512];
    while (stream->connected() || stream->available()) {
        int c = stream->readBytes(buff, sizeof(buff));
        if (c > 0) file.write(buff, c);
    }



    file.close();
    Serial.println("Saved: " + filename);
  } else {
    Serial.printf("Failed to download %s (HTTP %d)\n", filename.c_str(), httpCode);
    http.end();
    return false;
  }

  http.end();
  return true;
}

void downloadAllAudio(JsonArray cards) {
  for (JsonObject card : cards) {
    long long cardId = card["id"].as<long long>();

    // filenames for front and back audio
    String files[] = { String(cardId) + "_front.wav", String(cardId) + "_back.wav" };

    for (String file : files) {
      if (!SD.exists("/" + file)) {
        Serial.println("Downloading: " + file);
        if (!downloadFile(file)) {
          Serial.println("Failed: " + file);
        }
      } else {
        Serial.println("Already downloaded: " + file);
      }
    }
  }
}
