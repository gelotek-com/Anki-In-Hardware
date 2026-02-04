#include "setup_buttons.h"
#include "audio_setup.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "BUTTONS";


const int button1Pin = GPIO_NUM_1; // yellow
const int button2Pin = GPIO_NUM_0; // red
const int button3Pin = GPIO_NUM_2; // blue

void setupButtons()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    io_conf.pin_bit_mask =
        (1ULL << button1Pin) |
        (1ULL << button2Pin) |
        (1ULL << button3Pin);

    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Buttons configured");
}

void waitForButton(const char *message)
{
    ESP_LOGI(TAG, "%s", message);

    while (true) {
        if (gpio_get_level((gpio_num_t)button1Pin) == 0) {
            vTaskDelay(pdMS_TO_TICKS(300));
            return;
        }

        if (gpio_get_level((gpio_num_t)button3Pin) == 0) {
            vTaskDelay(pdMS_TO_TICKS(300));

            clearExtFiles();
            playAudio("/clear.wav", "/littlefs");

            playAudio("/downloading.wav", "/littlefs");
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}