idf_component_register(
    SRCS
        "main.cpp"
        "audio_setup.cpp"
        "http_setup.cpp"
        "wifi_setup.cpp"
        "setup_buttons.cpp"
        "setup_leds.cpp"

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_http_client
        esp_wifi
        esp_event
        nvs_flash
        esp_netif
        driver
        vfs
        littlefs
)
