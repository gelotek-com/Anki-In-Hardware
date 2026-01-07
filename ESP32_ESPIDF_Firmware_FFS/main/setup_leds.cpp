#include "setup_leds.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LEDS";


const gpio_num_t ledRed   = GPIO_NUM_20;
const gpio_num_t ledGreen = GPIO_NUM_21;
const gpio_num_t ledBlue  = GPIO_NUM_9;


void setupLeds()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    io_conf.pin_bit_mask =
        (1ULL << ledRed) |
        (1ULL << ledGreen) |
        (1ULL << ledBlue);

    gpio_config(&io_conf);
    gpio_set_level(ledGreen, 0);
    gpio_set_level(ledRed, 0);
    gpio_set_level(ledBlue, 0);

    ESP_LOGI(TAG, "LEDs initialized");
}
