#include <stdio.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/gpio.h"

#include <ArduinoJson.h>

// Your existing headers (to be converted later)
#include "wifi_setup.h"
#include "setup_leds.h"
#include "setup_buttons.h"
#include "http_setup.h"
#include "audio_setup.h"
#include "ext_flash.h"

static const char *TAG = "MAIN";


extern "C" void app_main(void)
{
    // Initialize NVS (required for WiFi, HTTP, etc.)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    ESP_LOGI(TAG, "System starting");

    // Hardware setup
    setupButtons();
    setupLeds();

    vTaskDelay(pdMS_TO_TICKS(1000));

    setupAudio(); // setup audio  + extflash
    setupWiFi();


    /*
    clearIntFiles();

    vTaskDelay(pdMS_TO_TICKS(1000));

    DownloadSpecificAudio("/response.wav");
    DownloadSpecificAudio("/start.wav");
    DownloadSpecificAudio("/clear.wav");
    DownloadSpecificAudio("/download.wav");
    DownloadSpecificAudio("/downloading.wav");
    DownloadSpecificAudio("/end.wav");
    */

    

    


    // Startup audio
    playAudio("/start.wav", "/littlefs");
    playAudio("/response.wav", "/littlefs");

    // Wait for user action
    waitForButton(
        ledGreen,
        ledBlue,
        ledRed,
        "Press YELLOW to start or BLUE to download the audio files"
    );

    // Get cards from server
    std::string cardData = HTTPboot();

    // JSON for HTTP output
    JsonDocument easeDoc;
    JsonArray easeArray = easeDoc["results"].to<JsonArray>();

    // JSON for FlashCards
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, cardData);
    if (err) {
        ESP_LOGE(TAG, "JSON parse failed: %s", err.c_str());
        return;
    }

    JsonArray cards = doc["cards"].as<JsonArray>();

    // Download all audio files
    downloadAllAudio_ext(cards);

    // Reset LEDs (because of waitForButton)
    gpio_set_level(ledGreen, 0);
    gpio_set_level(ledRed, 0);
    gpio_set_level(ledBlue, 0);

    playAudio("/download.wav", "/littlefs");

    // Study session
    for (JsonObject card : cards) {
        int64_t cardId = card["id"].as<long long>();
        std::string front = card["front"].as<std::string>();
        std::string back  = card["back"].as<std::string>();

        if (checkAudioFiles_ext(cardId)) {
            ESP_LOGI(TAG, "---- Next card ----");
            ESP_LOGI(TAG, "Front: %s", front.c_str());

            // Front
            gpio_set_level(ledBlue, 1);
            ESP_LOGI(TAG, "Press START to see the back.");
            playFrontAudio(button3Pin, button1Pin, cardId);
            gpio_set_level(ledBlue, 0);

            // Back
            ESP_LOGI(TAG, "Back: %s", back.c_str());
            ESP_LOGI(TAG, "Decide");

            gpio_set_level(ledGreen, 1);
            int ease = playBackAudio(
                button1Pin,
                button2Pin,
                button3Pin,
                cardId
            );
            gpio_set_level(ledGreen, 0);

            // Store result
            JsonObject r = easeArray.add<JsonObject>();
            r["card_id"] = cardId;
            r["ease"] = ease;
        }
    }

    // Send results back
    std::string jsonOut;
    serializeJson(easeDoc, jsonOut);

    ESP_LOGI(TAG, "Result JSON: %s", jsonOut.c_str());
    HTTPout(ledRed, jsonOut);

    // Session complete
    playAudio("/end.wav", "/littlefs");
    ESP_LOGI(TAG, "Study session complete.");

    gpio_set_level(ledGreen, 1);

    // app_main should not exit
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}