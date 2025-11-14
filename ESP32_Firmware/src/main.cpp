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

  waitForButton(button1Pin, ledGreen, "Press START to begin study.");

  //get the cards from the server
  String cardData = HTTPboot();

  playAudio("/start.wav");

  //json for HTTPout
  JsonDocument easeDoc;
  JsonArray easeArray = easeDoc["results"].to<JsonArray>();

  //json for FlashCards
  int ease;
  JsonDocument doc;
  deserializeJson(doc, cardData);
  JsonArray cards = doc["cards"].as<JsonArray>();

  //downloading mp3 files
  downloadAllAudio(cards);

  //study session
  for (JsonObject card : cards) {
    int64_t cardId = card["id"].as<long long>(); 
    String front = card["front"].as<String>();
    String back  = card["back"].as<String>();

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