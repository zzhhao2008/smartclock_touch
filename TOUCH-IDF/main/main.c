#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "st7789.h"
#include "fontx.h"
#include "lib/bmpfile.h"
#include "lib/decode_jpeg.h"
#include "lib/decode_png.h"
#include "lib/pngle.h"

#include "tests.c"

#define INTERVAL 100
#define WAIT vTaskDelay(INTERVAL)

static const char *TAG = "ST7789";

// You have to set these CONFIG value using menuconfig.
// #if 0
#define CONFIG_WIDTH 240
#define CONFIG_HEIGHT 320
#define CONFIG_MOSI_GPIO 45
#define CONFIG_SCLK_GPIO 3
#define CONFIG_CS_GPIO 14
#define CONFIG_DC_GPIO 47
#define CONFIG_RESET_GPIO 21
#define CONFIG_BL_GPIO 5
#define CONFIG_INVERSION 1
// #endif

void traceHeap()
{
	static uint32_t _free_heap_size = 0;
	if (_free_heap_size == 0)
		_free_heap_size = esp_get_free_heap_size();

	int _diff_free_heap_size = _free_heap_size - esp_get_free_heap_size();
	ESP_LOGI(__FUNCTION__, "_diff_free_heap_size=%d", _diff_free_heap_size);
	ESP_LOGI(__FUNCTION__, "esp_get_free_heap_size() : %6" PRIu32 "\n", esp_get_free_heap_size());
}

static void listSPIFFS(char *path)
{
	DIR *dir = opendir(path);
	assert(dir != NULL);
	while (true)
	{
		struct dirent *pe = readdir(dir);
		if (!pe)
			break;
		ESP_LOGI(__FUNCTION__, "d_name=%s d_ino=%d d_type=%x", pe->d_name, pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

esp_err_t mountSPIFFS(char *path, char *label, int max_files)
{
	esp_vfs_spiffs_conf_t conf = {
		.base_path = path,
		.partition_label = label,
		.max_files = max_files,
		.format_if_mount_failed = true};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK)
	{
		if (ret == ESP_FAIL)
		{
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		}
		else if (ret == ESP_ERR_NOT_FOUND)
		{
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		}
		else
		{
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return ret;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(conf.partition_label, &total, &used);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	}
	else
	{
		ESP_LOGI(TAG, "Mount %s to %s success", path, label);
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}

	return ret;
}

void ST7789(void *pvParameters)
{
	// set font file
	FontxFile fx16G[2];
	FontxFile fx24G[2];
	FontxFile fx32G[2];
	FontxFile fx32L[2];
	InitFontx(fx16G, "/fs/fonts/ILGH16XB.FNT", ""); // 8x16Dot Gothic
	InitFontx(fx24G, "/fs/fonts/ILGH24XB.FNT", ""); // 12x24Dot Gothic
	InitFontx(fx32G, "/fs/fonts/ILGH32XB.FNT", ""); // 16x32Dot Gothic
	InitFontx(fx32L, "/fs/fonts/LATIN32B.FNT", ""); // 16x32Dot Latin

	FontxFile fx16M[2];
	FontxFile fx24M[2];
	FontxFile fx32M[2];
	InitFontx(fx16M, "/fs/fonts/ILMH16XB.FNT", ""); // 8x16Dot Mincyo
	InitFontx(fx24M, "/fs/fonts/ILMH24XB.FNT", ""); // 12x24Dot Mincyo
	InitFontx(fx32M, "/fs/fonts/ILMH32XB.FNT", ""); // 16x32Dot Mincyo

	TFT_t dev;

	// Change SPI Clock Frequency
	spi_clock_speed(40000000);

	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);
	lcdInversionOn(&dev);

	char file[32];

	while (1)
	{
		traceHeap();

		FillTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		ColorBarTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		lcdFillScreen(&dev, WHITE);
		strcpy(file, "/fs/icons/battery01.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 0);
		strcpy(file, "/fs/icons/battery02.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 50);
		strcpy(file, "/fs/icons/battery03.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 100);
		strcpy(file, "/fs/icons/battery04.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 150);
		strcpy(file, "/fs/icons/battery05.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 200);
	}
}

void app_main(void)
{
	ESP_LOGI(TAG, "Initializing SPIFFS");
	// Maximum files that could be open at the same time is 7.
	ESP_ERROR_CHECK(mountSPIFFS("/fs", "MFS", 7));
	listSPIFFS("/fs/");

	xTaskCreate(ST7789, "ST7789", 1024 * 6, NULL, 2, NULL);
}
