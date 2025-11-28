#include <Arduino.h>
#include <ArduinoJson.h>

#include "wifi_setup.h"
#include "setup_leds.h"
#include "setup_buttons.h"
#include "http_setup.h"
#include "audio_setup.h"

void setup() {
  Serial.begin(115200);
  setupButtons();
  setupLeds();
  delay(1000);
  setupAudio();
  setupWiFi();

  playAudio("/start.wav");
  playAudio("/response.wav");

  //blue: remove audio files -- playaudio("downloading") -- playaudio 
  waitForButton(ledGreen, ledBlue, ledRed, "Press YELLOW to start or BLUE to download the audio files");

  //get the cards from the server
  String cardData = HTTPboot();

  //json for HTTPout
  JsonDocument easeDoc;
  JsonArray easeArray = easeDoc["results"].to<JsonArray>();

  //json for FlashCards
  int ease;
  JsonDocument doc;
  deserializeJson(doc, cardData);
  JsonArray cards = doc["cards"].as<JsonArray>();

  //it will say "starting download" when the user press the blue button

  //downloading mp3 files
  downloadAllAudio(cards);

  //because of waitforbutton function
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledRed, LOW);
  digitalWrite(ledBlue, LOW);

  playAudio("/download.wav");

  //study session
  for (JsonObject card : cards) {
    int64_t cardId = card["id"].as<long long>(); 
    String front = card["front"].as<String>();
    String back  = card["back"].as<String>();

    //if the esp hasn't download all the files, the study session can be incomplete
    if (checkAudioFiles(cardId)){
      Serial.println("---- Next card ----");

      //front
      Serial.println("Front: " + front);
      digitalWrite(ledBlue, HIGH);
      Serial.println("Press START to see the back.");
      playFrontAudio(button3Pin, button1Pin, cardId); //playAudio() + waitForButton()
      digitalWrite(ledBlue, LOW);

      //back
      Serial.println("Back: " + back);
      Serial.println("decide");
      digitalWrite(ledGreen, HIGH);
      ease = playBackAudio(button1Pin, button2Pin, button3Pin, cardId); //playAudio() + waitForStudy()
      digitalWrite(ledGreen, LOW);

      //storing results
      JsonObject r = easeArray.add<JsonObject>();
      r["card_id"] = cardId;
      r["ease"] = ease;
    }
  }

  //sending back the results
  String jsonOut;
  serializeJson(easeDoc, jsonOut);
  Serial.println(jsonOut);
  HTTPout(ledRed, jsonOut);

  //session complete
  playAudio("/end.wav");
  Serial.println("Study session complete.");
  digitalWrite(ledGreen, HIGH); 
}

void loop() {}
