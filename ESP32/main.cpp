#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "";
const char* password = "";
const char* server = "";

const int button1Pin = 5; //yellow button
const int button2Pin = 6; //red button

void waitForButton(int pin, const char* message) {
  Serial.println(message);
  while (digitalRead(pin) == HIGH) {
    delay(50);
    yield();
  }
  delay(300);
}

int waitForStudy(int pin,int pin2, const char* message) {
  Serial.println(message);
  while (true) {
    if (digitalRead(pin) == LOW){ //yellow for 1
      delay(300);
      return 1;
    }
    if (digitalRead(pin2) == LOW){ //red for 4
      delay(300);
      return 4;
    }
    delay(50);
    yield();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  waitForButton(button1Pin, "Press START to begin study.");


  String cardData;
  {
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
  }



  JsonDocument easeDoc;
  JsonArray easeArray = easeDoc["results"].to<JsonArray>();

  int ease;
  JsonDocument doc;
  deserializeJson(doc, cardData);

  JsonArray cards = doc["cards"].as<JsonArray>();

  for (JsonObject card : cards) {
    int64_t cardId = card["id"].as<long long>(); 
    String front = card["front"].as<String>();
    String back  = card["back"].as<String>();

    Serial.println("Front: " + front);
    waitForButton(button1Pin, "Press START to see the back.");

    Serial.println("Back: " + back);
    delay(1000);
    ease = waitForStudy(button1Pin, button2Pin, "Decide");

    JsonObject r = easeArray.add<JsonObject>();
    r["card_id"] = cardId;
    r["ease"] = ease;

    Serial.println("---- Next card ----");
  }

  String jsonOut;
  serializeJson(easeDoc, jsonOut);
  Serial.println(jsonOut);

  {
    HTTPClient http;
    http.begin(String(server) + "/ease");
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(jsonOut);
    String resp = http.getString();
    Serial.printf("Ease response [%d]: %s\n", code, resp.c_str());
    http.end();
  }

  Serial.println("Study session complete.");
}

void loop() {}

