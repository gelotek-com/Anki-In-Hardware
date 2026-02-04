#pragma once

#include <string>
#include <cstdint>

#include <ArduinoJson.h>

// Get cards JSON from server
std::string HTTPboot();

// Send study results
void HTTPout(const std::string &jsonOut);

// Audio downloads
void downloadAllAudio_ext(JsonArray cards);
void DownloadSpecificAudio(const std::string &path);
