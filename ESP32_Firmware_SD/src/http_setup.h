#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>


String HTTPboot();
void HTTPout(int ledRed, String jsonOut);
void downloadAllAudio(JsonArray cards);