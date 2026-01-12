#include "audio_setup.h"
#include "ext_flash.h"

#include <stdio.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
//#include "esp_vfs_littlefs.h"
#include "esp_vfs.h"
#include "esp_littlefs.h"

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

static const char *TAG = "AUDIO";

#define AUDIO_PIN      GPIO_NUM_3
#define PWM_CHANNEL    LEDC_CHANNEL_0
#define PWM_TIMER      LEDC_TIMER_0
#define PWM_FREQ       80000
#define PWM_RES        LEDC_TIMER_8_BIT
#define SAMPLE_RATE    16000

static FILE *wavFile = nullptr;

void setupAudio()
{
    vTaskDelay(pdMS_TO_TICKS(200));

    mount_extflash(); //ADDED

    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "Initializing LittleFS...");
    vTaskDelay(pdMS_TO_TICKS(200));

    esp_vfs_littlefs_conf_t conf = {};
    conf.base_path = "/littlefs";
    conf.partition_label = "storage"; //littlefs
    conf.format_if_mount_failed = true;
    conf.read_only = false;
    conf.grow_on_mount = false;
    conf.dont_mount = false;


    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LittleFS mount failed (%s)", esp_err_to_name(ret));
        while (true) vTaskDelay(portMAX_DELAY);
    }

    ESP_LOGI(TAG, "LittleFS OK!");

    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.timer_num = PWM_TIMER;
    timer.duty_resolution = PWM_RES;
    timer.freq_hz = PWM_FREQ;
    timer.clk_cfg = LEDC_AUTO_CLK;
    timer.deconfigure = false;

    ledc_timer_config(&timer);

    // LEDC Channel

    ledc_channel_config_t channel = {};
    channel.gpio_num = AUDIO_PIN;
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel = PWM_CHANNEL;
    channel.timer_sel = PWM_TIMER;
    channel.duty = 0;
    channel.hpoint = 0;
    channel.intr_type = LEDC_INTR_DISABLE;
    channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;


    ledc_channel_config(&channel);
}

static void skipWavHeader(FILE *file)
{
    fseek(file, 44, SEEK_SET);
}

void clearExtFiles(){
    DIR *dir = opendir("/extflash");
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        std::string path = "/extflash/" + name;
        ESP_LOGI(TAG, "Removing: %s", path.c_str());
        unlink(path.c_str());
        
    }

    closedir(dir);
    ESP_LOGI(TAG, "Finished clearing External Flash");
}

void clearIntFiles (){
    DIR *dir = opendir("/littlefs");
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        std::string path = "/littlefs/" + name;
        ESP_LOGI(TAG, "Removing: %s", path.c_str());
        unlink(path.c_str());
        
    }

    closedir(dir);
    ESP_LOGI(TAG, "Finished clearing LittleFS");
}

void playAudio(const std::string &path, const std::string &storage)
{
    std::string fullPath = storage + path;
    wavFile = fopen(fullPath.c_str(), "rb");

    if (!wavFile) {
        ESP_LOGE(TAG, "Audio file not found: %s", fullPath.c_str());
        return;
    }

    skipWavHeader(wavFile);

    uint8_t buffer[512];

    while (!feof(wavFile)) {
        size_t bytesRead = fread(buffer, 1, sizeof(buffer), wavFile);

        for (size_t i = 0; i < bytesRead; i++) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, buffer[i]);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
            esp_rom_delay_us(1000000 / SAMPLE_RATE);
        }
    }

    fclose(wavFile);
    wavFile = nullptr;

    ESP_LOGI(TAG, "Done playing: %s", path.c_str());
}

void playFrontAudio(int button3Pin, int button1Pin, int64_t cardId)
{
    std::string file = "/" + std::to_string(cardId) + "_front.wav";
    playAudio(file, "/extflash");

    while (true) {
        if (gpio_get_level((gpio_num_t)button3Pin) == 0) {
            playAudio(file, "/extflash");
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        if (gpio_get_level((gpio_num_t)button1Pin) == 0) {
            vTaskDelay(pdMS_TO_TICKS(300));
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

int playBackAudio(int button1Pin, int button2Pin, int button3Pin, int64_t cardId)
{
    std::string file = "/" + std::to_string(cardId) + "_back.wav";
    playAudio(file, "/extflash");

    while (true) {
        if (gpio_get_level((gpio_num_t)button3Pin) == 0) {
            playAudio(file, "/extflash");
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        if (gpio_get_level((gpio_num_t)button1Pin) == 0) {
            vTaskDelay(pdMS_TO_TICKS(300));
            return 1;
        }
        if (gpio_get_level((gpio_num_t)button2Pin) == 0) {
            vTaskDelay(pdMS_TO_TICKS(300));
            return 4;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

bool checkAudioFiles_ext(int64_t cardId)
{
    std::string front = "/extflash/" + std::to_string(cardId) + "_front.wav";
    std::string back  = "/extflash/" + std::to_string(cardId) + "_back.wav";

    FILE *f1 = fopen(front.c_str(), "r");
    FILE *f2 = fopen(back.c_str(), "r");

    if (f1 && f2) {
        fclose(f1);
        fclose(f2);
        ESP_LOGI(TAG, "Audio files exist");
        return true;
    }

    if (f1) fclose(f1);
    if (f2) fclose(f2);

    ESP_LOGW(TAG, "Audio files do not exist, this card cannot be studied");
    return false;
}