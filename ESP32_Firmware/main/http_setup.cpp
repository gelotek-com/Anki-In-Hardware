#include "http_setup.h"

#include <string>
#include <vector>

#include "esp_log.h"
#include "esp_http_client.h"
//#include "esp_vfs_littlefs.h"
#include "esp_vfs.h"
#include "esp_littlefs.h"

#include <ArduinoJson.h>

static const char *TAG = "HTTP";

// Server address
static const char *SERVER = "http://192.168.1.100:8000";


static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (evt->data_len > 0) {
                FILE *file = (FILE *)evt->user_data;
                fwrite(evt->data, 1, evt->data_len, file);
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

struct HttpBuffer {
    std::string *response;
};


static esp_err_t http_event_handler_json(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA &&
        evt->data &&
        evt->data_len > 0 &&
        evt->user_data) {

        HttpBuffer *buf = (HttpBuffer *)evt->user_data;
        buf->response->append((const char*)evt->data, evt->data_len);
    }
    return ESP_OK;
}

std::string HTTPboot()
{
    std::string response;

    HttpBuffer buf;
    buf.response = &response;

    esp_http_client_config_t config = {};
    config.url = "http://192.168.1.100:8000/boot";
    config.method = HTTP_METHOD_POST;
    config.event_handler = http_event_handler_json;
    config.user_data = &buf;
    config.timeout_ms = 10000;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    const char *post_data = "{\"device\":\"esp32\"}";
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);

    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200) {
        ESP_LOGE(TAG, "HTTP /boot failed (%d): %s", status, esp_err_to_name(err));
        return {};
    }

    ESP_LOGI(TAG, "Boot JSON: %s", response.c_str());
    return response;
}


static esp_err_t http_event_handler_capture(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0 && evt->user_data) {
        HttpBuffer *buf = (HttpBuffer *)evt->user_data;
        buf->response->append((const char *)evt->data, evt->data_len);
    }
    return ESP_OK;
}

void HTTPout(const std::string &jsonOut)
{
    std::string response;

    HttpBuffer buf;
    buf.response = &response;

    esp_http_client_config_t config = {};
    config.url = "http://192.168.1.100:8000/ease";
    config.event_handler = http_event_handler_capture;
    config.method = HTTP_METHOD_POST;
    config.user_data = &buf;  // attach buffer to user_data

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, jsonOut.c_str(), jsonOut.length());

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Ease response [%d]: %s", status, response.c_str());
    } else {
        ESP_LOGE(TAG, "HTTPout failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}


bool downloadFile_ext(const std::string &filename)
{
    std::string path = "/extflash/" + filename;
    std::string url  = std::string(SERVER) + "/audio/" + filename;

    FILE *check = fopen(path.c_str(), "r");
    if (check) {
        fclose(check);
        ESP_LOGI(TAG, "Already downloaded: %s", filename.c_str());
        return true;
    }

    FILE *file = fopen(path.c_str(), "wb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", path.c_str());
        return false;
    }

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.event_handler = http_event_handler;
    config.user_data = file;
    config.timeout_ms = 10000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    fclose(file);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || esp_http_client_get_status_code(client) != 200) {
        ESP_LOGE(TAG, "Failed to download %s", filename.c_str());
        return false;
    }

    ESP_LOGI(TAG, "Saved: %s", filename.c_str());
    return true;
}


void downloadAllAudio_ext(JsonArray cards)
{
    for (JsonObject card : cards) {
        int64_t cardId = card["id"].as<long long>();

        std::string files[] = {
            std::to_string(cardId) + "_front.wav",
            std::to_string(cardId) + "_back.wav"
        };

        for (auto &file : files) {
            std::string path = "/extflash/" + file;

            FILE *f = fopen(path.c_str(), "r");
            if (f) {
                fclose(f);
                ESP_LOGI(TAG, "Already downloaded: %s", file.c_str());
                continue;
            }

            ESP_LOGI(TAG, "Downloading: %s", file.c_str());
            if (!downloadFile_ext(file)) {
                ESP_LOGE(TAG, "Failed: %s", file.c_str());
            }
        }
    }
}

void DownloadSpecificAudio(const std::string &path)
{
    std::string fullPath = "/littlefs" + path;

    // Check if file exists
    FILE *check = fopen(fullPath.c_str(), "r");
    if (check) {
        fclose(check);
        ESP_LOGI(TAG, "Already downloaded: %s", path.c_str());
        return;
    }

    std::string url = std::string(SERVER) + "/audio" + path;

    FILE *file = fopen(fullPath.c_str(), "wb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", fullPath.c_str());
        return;
    }

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.event_handler = http_event_handler;
    config.user_data = file;
    config.timeout_ms = 10000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    fclose(file);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || esp_http_client_get_status_code(client) != 200) {
        ESP_LOGE(TAG, "Failed to download %s", path.c_str());
        return;
    }

    ESP_LOGI(TAG, "Saved: %s", path.c_str());
}