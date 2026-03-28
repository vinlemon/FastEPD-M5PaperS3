//
// FastEPD SD File Explorer for M5PaperS3
// ESP-IDF port of the Arduino sd_file_explorer example.
//
// This example demonstrates:
// - Mounting a microSD card via SPI (M5PaperS3 SD pin mapping)
// - Listing directory contents recursively via UART console
// - Displaying JPEG images from SD using JPEGDEC
//
// M5PaperS3 SD Card SPI pins:
//   CS   = GPIO 47
//   SCK  = GPIO 39
//   MOSI = GPIO 38
//   MISO = GPIO 40
//
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "../../../../src/FastEPD.h"
#include "JPEGDEC.h"

static const char *TAG = "sd_explorer";

// M5PaperS3 SD Card SPI pin mapping
#define SD_CS_PIN   GPIO_NUM_47
#define SD_SCK_PIN  GPIO_NUM_39
#define SD_MOSI_PIN GPIO_NUM_38
#define SD_MISO_PIN GPIO_NUM_40
#define SD_SPI_HOST SPI2_HOST
#define SD_MOUNT_POINT "/sdcard"

FASTEPD epaper;
JPEGIMAGE jpg;

//
// JPEGDEC C-API forward declarations
//
extern "C" {
int JPEG_openRAM(JPEGIMAGE *pJPEG, uint8_t *pData, int iDataSize, JPEG_DRAW_CALLBACK *pfnDraw);
int JPEG_openFile(JPEGIMAGE *pJPEG, const char *szFilename, JPEG_DRAW_CALLBACK *pfnDraw);
void JPEG_setPixelType(JPEGIMAGE *pJPEG, int iType);
int JPEG_decode(JPEGIMAGE *pJPEG, int x, int y, int iOptions);
void JPEG_close(JPEGIMAGE *pJPEG);
int JPEG_getWidth(JPEGIMAGE *pJPEG);
int JPEG_getHeight(JPEGIMAGE *pJPEG);
}

// JPEG draw callback: converts 8-bit gray to 4-bit packed framebuffer
int JPEGDraw(JPEGDRAW *pDraw)
{
    int x, y, iPitch = epaper.width() / 2;
    uint8_t *s, *d, *pBuffer = epaper.currentBuffer();
    for (y = 0; y < pDraw->iHeight; y++) {
        d = &pBuffer[((pDraw->y + y) * iPitch) + (pDraw->x / 2)];
        s = (uint8_t *)pDraw->pPixels;
        s += (y * pDraw->iWidth);
        for (x = 0; x < pDraw->iWidth; x += 2) {
            *d++ = (s[0] & 0xf0) | (s[1] >> 4);
            s += 2;
        }
    }
    return 1;
}

// Mount SD card via SPI
static sdmmc_card_t *s_card = NULL;

static esp_err_t mount_sd_card(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 8,
        .allocation_unit_size = 16 * 1024
    };

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_PIN;
    slot_config.host_id = SD_SPI_HOST;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    sdmmc_card_print_info(stdout, s_card);
    return ESP_OK;
}

// List directory contents to stdout
static void list_dir(const char *path, int depth)
{
    DIR *dir = opendir(path);
    if (!dir) {
        printf("Cannot open: %s\n", path);
        return;
    }
    struct dirent *entry;
    char indent[64] = "";
    for (int i = 0; i < depth && i < 10; i++) strncat(indent, "  ", 63);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // skip hidden

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                printf("%s[DIR] %s\n", indent, entry->d_name);
                if (depth < 2) list_dir(full_path, depth + 1); // limit recursion
            } else {
                printf("%s%s  (%ld bytes)\n", indent, entry->d_name, (long)st.st_size);
            }
        }
    }
    closedir(dir);
}

// Display a JPEG from SD card
static void display_jpeg(const char *filepath)
{
    printf("Opening JPEG: %s\n", filepath);
    epaper.setMode(BB_MODE_4BPP);
    epaper.fillScreen(0xf);

    if (JPEG_openFile(&jpg, filepath, JPEGDraw)) {
        JPEG_setPixelType(&jpg, EIGHT_BIT_GRAYSCALE);
        int w = JPEG_getWidth(&jpg);
        int h = JPEG_getHeight(&jpg);
        // Center on display (keeping decode at 0,0 for simplicity)
        JPEG_decode(&jpg, 0, 0, 0);
        JPEG_close(&jpg);
        epaper.fullUpdate();
        printf("Displayed JPEG (%d x %d)\n", w, h);
    } else {
        printf("Failed to open JPEG\n");
    }
}

extern "C" void app_main(void)
{
    printf("\n=== M5PaperS3 SD File Explorer ===\n");

    printf("Initializing e-paper panel...\n");
    int rc = epaper.initPanel(BB_PANEL_M5PAPERS3);
    if (rc != BBEP_SUCCESS) {
        ESP_LOGE(TAG, "Failed to init panel: %d", rc);
        return;
    }

    printf("Mounting SD card (SPI)...\n");
    if (mount_sd_card() != ESP_OK) {
        // Show error on display
        epaper.setMode(BB_MODE_1BPP);
        epaper.fillScreen(BBEP_WHITE);
        epaper.setFont(FONT_12x16);
        epaper.setTextColor(BBEP_BLACK);
        epaper.drawString("SD card mount failed!", 10, 100);
        epaper.fullUpdate();
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("SD card mounted successfully!\n\n");

    // List root directory
    printf("=== SD Card Contents ===\n");
    list_dir(SD_MOUNT_POINT, 0);
    printf("========================\n\n");

    // Show a welcome screen on e-paper
    epaper.setMode(BB_MODE_1BPP);
    epaper.fillScreen(BBEP_WHITE);
    epaper.setFont(FONT_12x16);
    epaper.setTextColor(BBEP_BLACK);
    epaper.drawString("SD Explorer - M5PaperS3", 10, 50);
    epaper.drawString("Check serial for file list", 10, 90);
    epaper.drawString("Place .jpg files on SD root", 10, 130);
    epaper.fullUpdate();

    // If any .jpg files exist at root, display the first one
    DIR *root = opendir(SD_MOUNT_POINT);
    if (root) {
        struct dirent *entry;
        while ((entry = readdir(root)) != NULL) {
            int len = strlen(entry->d_name);
            if (len > 4) {
                const char *ext = &entry->d_name[len - 4];
                if (strcasecmp(ext, ".jpg") == 0) {
                    char jpg_path[512];
                    snprintf(jpg_path, sizeof(jpg_path), "%s/%s", SD_MOUNT_POINT, entry->d_name);
                    display_jpeg(jpg_path);
                    vTaskDelay(pdMS_TO_TICKS(5000)); // show for 5 seconds between images
                }
            }
        }
        closedir(root);
    }

    printf("Done! Entering idle loop.\n");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
