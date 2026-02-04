//from espidf example: https://github.com/espressif/esp-idf/tree/master/examples/storage/fatfs/ext_flash

#include "ext_flash.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include "soc/spi_pins.h"

#define EXAMPLE_FLASH_FREQ_MHZ      40

static const char *TAG = "example";

#define HOST_ID  SPI2_HOST
#define PIN_MOSI 6
#define PIN_MISO 5
#define PIN_CLK  4
#define PIN_CS   7
#define PIN_WP  -1
#define PIN_HD  -1
#define SPI_DMA_CHAN SPI_DMA_CH_AUTO

// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

// Mount path for the partition
const char *base_path = "/extflash";

static esp_flash_t* example_init_ext_flash(void);
static const esp_partition_t* example_add_partition(esp_flash_t* ext_flash, const char* partition_label);
static void example_list_data_partitions(void);
static bool example_mount_fatfs(const char* partition_label);
void mount_extflash();


void mount_extflash(){
    // Set up SPI bus and initialize the external SPI Flash chip
    esp_flash_t* flash = example_init_ext_flash();
    if (flash == NULL) {
        return;
    }

    // Add the entire external flash chip as a partition
    const char *partition_label = "ext_flash";
    example_add_partition(flash, partition_label);

    // List the available partitions
    example_list_data_partitions();

    // Initialize FAT FS in the partition
    if (!example_mount_fatfs(partition_label)) {
        return;
    }
}

static esp_flash_t* example_init_ext_flash(void)
{
    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = PIN_MOSI;
    bus_config.miso_io_num = PIN_MISO;
    bus_config.sclk_io_num = PIN_CLK;
    bus_config.quadhd_io_num = PIN_HD;
    bus_config.quadwp_io_num = PIN_WP;


    esp_flash_spi_device_config_t device_config = {};
    device_config.host_id = HOST_ID;
    device_config.cs_id = 0;
    device_config.cs_io_num = PIN_CS;
    device_config.io_mode = SPI_FLASH_DIO;
    device_config.freq_mhz = EXAMPLE_FLASH_FREQ_MHZ;


    ESP_LOGI(TAG, "Initializing external SPI Flash");
    ESP_LOGI(TAG, "Pin assignments:");
    ESP_LOGI(TAG, "MOSI: %2d   MISO: %2d   SCLK: %2d   CS: %2d",
        bus_config.mosi_io_num, bus_config.miso_io_num,
        bus_config.sclk_io_num, device_config.cs_io_num
    );

    // Initialize the SPI bus
    ESP_LOGI(TAG, "DMA CHANNEL: %d", SPI_DMA_CHAN);
    ESP_ERROR_CHECK(spi_bus_initialize(HOST_ID, &bus_config, SPI_DMA_CHAN));

    // Add device to the SPI bus
    esp_flash_t* ext_flash;
    ESP_ERROR_CHECK(spi_bus_add_flash_device(&ext_flash, &device_config));

    // Probe the Flash chip and initialize it
    esp_err_t err = esp_flash_init(ext_flash);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external Flash: %s (0x%x)", esp_err_to_name(err), err);
        return NULL;
    }

    // Print out the ID and size
    uint32_t id;
    ESP_ERROR_CHECK(esp_flash_read_id(ext_flash, &id));
    ESP_LOGI(TAG, "Initialized external Flash, size=%" PRIu32 " KB, ID=0x%" PRIx32, ext_flash->size / 1024, id);

    return ext_flash;
}

static const esp_partition_t* example_add_partition(esp_flash_t* ext_flash, const char* partition_label)
{
    ESP_LOGI(TAG, "Adding external Flash as a partition, label=\"%s\", size=%" PRIu32 " KB", partition_label, ext_flash->size / 1024);
    const esp_partition_t* fat_partition;
    const size_t offset = 0;
    ESP_ERROR_CHECK(esp_partition_register_external(ext_flash, offset, ext_flash->size, partition_label, ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, &fat_partition));

    // Erase space of partition on the external flash chip
    ESP_LOGI(TAG, "Erasing partition range, offset=%u size=%" PRIu32 " KB", offset, ext_flash->size / 1024);
    //ESP_ERROR_CHECK(esp_partition_erase_range(fat_partition, offset, ext_flash->size));

    return fat_partition;
}

static void example_list_data_partitions(void)
{
    ESP_LOGI(TAG, "Listing data partitions:");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);

    for (; it != NULL; it = esp_partition_next(it)) {
        const esp_partition_t *part = esp_partition_get(it);
        ESP_LOGI(TAG, "- partition '%s', subtype %d, offset 0x%" PRIx32 ", size %" PRIu32 " kB",
        part->label, part->subtype, part->address, part->size / 1024);
    }

    esp_partition_iterator_release(it);
}

static bool example_mount_fatfs(const char* partition_label)
{
    ESP_LOGI(TAG, "Mounting FAT filesystem");
    esp_vfs_fat_mount_config_t mount_config = {};
    mount_config.max_files = 4;
    mount_config.format_if_mount_failed = true; // to avoid esp_partition_erase_range(fat_partition, offset, ext_flash->size)
    mount_config.allocation_unit_size = CONFIG_WL_SECTOR_SIZE;
    mount_config.use_one_fat = false;

    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(base_path, partition_label, &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return false;
    }
    return true;
}