#include "audio_setup.h"
#include "ext_flash.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
//#include "esp_vfs_littlefs.h"
#include "esp_vfs.h"
#include "esp_littlefs.h"

//#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

#include "driver/i2s_std.h"


#define EXAMPLE_STD_BCLK_IO1   GPIO_NUM_21
#define EXAMPLE_STD_WS_IO1     GPIO_NUM_10
#define EXAMPLE_STD_DOUT_IO1   GPIO_NUM_20
#define EXAMPLE_STD_DIN_IO1    GPIO_NUM_NC

#define EXAMPLE_BUFF_SIZE               2048
static i2s_chan_handle_t                tx_chan; 

static const char *TAG = "AUDIO";


void playAudio(const std::string &path, const std::string &storage)
{
    //printf("Playback started\n");
    std::string fullPath = storage + path;
    FILE *f = fopen(fullPath.c_str(), "rb");
    if (!f) {
        printf("Failed to open audio file\n");
        vTaskDelete(NULL);
        return;
    }

    fseek(f, 44, SEEK_SET);

    uint8_t *buf = (uint8_t *)malloc(EXAMPLE_BUFF_SIZE);
    if (!buf) {
        fclose(f);
        vTaskDelete(NULL);
        return;
    }

    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    size_t bytes_read = 0;
    
    while ((bytes_read = fread(buf, 1, EXAMPLE_BUFF_SIZE, f)) > 0) {
        //printf("Read %d bytes\n", bytes_read);
        size_t written = 0;
        esp_err_t err = i2s_channel_write(tx_chan, buf, bytes_read, &written, portMAX_DELAY);
        //printf("Written %d bytes\n", written);
        if (err != ESP_OK) {
            printf("I2S write failed: %d\n", err);
            break;
        }
    }

    //printf("Playback finished\n");
    i2s_channel_disable(tx_chan);
    free(buf);
    fclose(f);
    ESP_LOGI(TAG, "Done playing: %s", path.c_str());
}

void i2s_example_init_std_simplex(void)
{
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));

    i2s_std_config_t tx_std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = EXAMPLE_STD_BCLK_IO1,
            .ws   = EXAMPLE_STD_WS_IO1,
            .dout = EXAMPLE_STD_DOUT_IO1,
            .din  = EXAMPLE_STD_DIN_IO1,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_std_cfg));
}

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

    i2s_example_init_std_simplex();
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