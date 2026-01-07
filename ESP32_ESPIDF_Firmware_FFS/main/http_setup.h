#pragma once

#include <string>
#include <cstdint>

#include <ArduinoJson.h>

// Boot: get cards JSON from server
std::string HTTPboot();

// Send study results
void HTTPout(int ledRed, const std::string &jsonOut);

// Audio downloads
void downloadAllAudio(JsonArray cards);
void DownloadSpecificAudio(const std::string &path);
