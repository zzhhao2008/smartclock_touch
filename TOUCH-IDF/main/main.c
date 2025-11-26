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

#define INTERVAL 400
#define WAIT vTaskDelay(INTERVAL)

static const char *TAG = "ST7789";

// You have to set these CONFIG value using menuconfig.
#if 0
#define CONFIG_WIDTH  240
#define CONFIG_HEIGHT 240
#define CONFIG_MOSI_GPIO 23
#define CONFIG_SCLK_GPIO 18
#define CONFIG_CS_GPIO -1
#define CONFIG_DC_GPIO 19
#define CONFIG_RESET_GPIO 15
#define CONFIG_BL_GPIO -1
#endif

void traceHeap() {
	static uint32_t _free_heap_size = 0;
	if (_free_heap_size == 0) _free_heap_size = esp_get_free_heap_size();

	int _diff_free_heap_size = _free_heap_size - esp_get_free_heap_size();
	ESP_LOGI(__FUNCTION__, "_diff_free_heap_size=%d", _diff_free_heap_size);
	ESP_LOGI(__FUNCTION__, "esp_get_free_heap_size() : %6"PRIu32"\n", esp_get_free_heap_size() );
#if 0
	printf("esp_get_minimum_free_heap_size() : %6"PRIu32"\n", esp_get_minimum_free_heap_size() );
	printf("xPortGetFreeHeapSize() : %6zd\n", xPortGetFreeHeapSize() );
	printf("xPortGetMinimumEverFreeHeapSize() : %6zd\n", xPortGetMinimumEverFreeHeapSize() );
	printf("heap_caps_get_free_size(MALLOC_CAP_32BIT) : %6d\n", heap_caps_get_free_size(MALLOC_CAP_32BIT) );
	// that is the amount of stack that remained unused when the task stack was at its greatest (deepest) value.
	printf("uxTaskGetStackHighWaterMark() : %6d\n", uxTaskGetStackHighWaterMark(NULL));
#endif
}

void ST7789(void *pvParameters)
{
	// set font file
	FontxFile fx16G[2];
	FontxFile fx24G[2];
	FontxFile fx32G[2];
	FontxFile fx32L[2];
	InitFontx(fx16G,"/fonts/ILGH16XB.FNT",""); // 8x16Dot Gothic
	InitFontx(fx24G,"/fonts/ILGH24XB.FNT",""); // 12x24Dot Gothic
	InitFontx(fx32G,"/fonts/ILGH32XB.FNT",""); // 16x32Dot Gothic
	InitFontx(fx32L,"/fonts/LATIN32B.FNT",""); // 16x32Dot Latin

	FontxFile fx16M[2];
	FontxFile fx24M[2];
	FontxFile fx32M[2];
	InitFontx(fx16M,"/fonts/ILMH16XB.FNT",""); // 8x16Dot Mincyo
	InitFontx(fx24M,"/fonts/ILMH24XB.FNT",""); // 12x24Dot Mincyo
	InitFontx(fx32M,"/fonts/ILMH32XB.FNT",""); // 16x32Dot Mincyo
	
	TFT_t dev;

	// Change SPI Clock Frequency
	//spi_clock_speed(40000000); // 40MHz
	//spi_clock_speed(60000000); // 60MHz

	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

#if CONFIG_INVERSION
	ESP_LOGI(TAG, "Enable Display Inversion");
	//lcdInversionOn(&dev);
	lcdInversionOff(&dev);
#endif

	char file[32];
#if 0
	while (1) {
		FillTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;
		ArrowTest(&dev, fx16G, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;
	}
#endif

	while(1) {
		traceHeap();

		FillTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		ColorBarTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		ArrowTest(&dev, fx16G, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		LineTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		CircleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		RoundRectTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		if (dev._use_frame_buffer == false) {
			RectAngleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
			WAIT;

			TriangleTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
			WAIT;
		}

		if (CONFIG_WIDTH >= 240) {
			DirectionTest(&dev, fx24G, CONFIG_WIDTH, CONFIG_HEIGHT);
		} else {
			DirectionTest(&dev, fx16G, CONFIG_WIDTH, CONFIG_HEIGHT);
		}
		WAIT;

		if (CONFIG_WIDTH >= 240) {
			HorizontalTest(&dev, fx24G, CONFIG_WIDTH, CONFIG_HEIGHT);
		} else {
			HorizontalTest(&dev, fx16G, CONFIG_WIDTH, CONFIG_HEIGHT);
		}
		WAIT;

		if (CONFIG_WIDTH >= 240) {
			VerticalTest(&dev, fx24G, CONFIG_WIDTH, CONFIG_HEIGHT);
		} else {
			VerticalTest(&dev, fx16G, CONFIG_WIDTH, CONFIG_HEIGHT);
		}
		WAIT;

		FillRectTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		ColorTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		CodeTest(&dev, fx32G, CONFIG_WIDTH, CONFIG_HEIGHT, 0xA0, 0xFF);
		WAIT;

		CodeTest(&dev, fx32L, CONFIG_WIDTH, CONFIG_HEIGHT, 0xA0, 0xFF);
		WAIT;

		strcpy(file, "/images/image.bmp");
		BMPTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		strcpy(file, "/images/esp32.jpeg");
		JPEGTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		strcpy(file, "/images/esp_logo.png");
		PNGTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		if (dev._use_frame_buffer == true) {
			WrapArroundTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
			WAIT;

			ImageMoveTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
			WAIT;

			ImageInversionTest(&dev, CONFIG_WIDTH, CONFIG_HEIGHT);
			WAIT;

#if 0
			CursorTest(&dev, fx32G, CONFIG_WIDTH, CONFIG_HEIGHT);
			WAIT;
#endif
			TextBoxTest(&dev, fx32G, CONFIG_WIDTH, CONFIG_HEIGHT);
			WAIT;
		}

		strcpy(file, "/images/qrcode.bmp");
		QRTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);
		WAIT;

		lcdSetFontDirection(&dev, 0);
		lcdFillScreen(&dev, WHITE);
		strcpy(file, "/icons/battery01.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 0);
		strcpy(file, "/icons/battery02.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 50);
		strcpy(file, "/icons/battery03.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 100);
		strcpy(file, "/icons/battery04.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 150);
		strcpy(file, "/icons/battery05.png");
		IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, 0, 200);

		if (CONFIG_WIDTH > 160) {
			strcpy(file, "/icons/battery06.png");
			IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, (CONFIG_WIDTH/2)-1, 0);
			strcpy(file, "/icons/battery07.png");
			IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, (CONFIG_WIDTH/2)-1, 50);
			strcpy(file, "/icons/battery08.png");
			IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, (CONFIG_WIDTH/2)-1, 100);
			strcpy(file, "/icons/battery11.png");
			IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, (CONFIG_WIDTH/2)-1, 150);
			strcpy(file, "/icons/battery12.png");
			IconTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT, (CONFIG_WIDTH/2)-1, 200);
		}
		WAIT;

		// Multi Font Test
		uint16_t color;
		uint8_t ascii[40];
		uint16_t margin = 10;
		lcdFillScreen(&dev, BLACK);
		color = WHITE;
		lcdSetFontDirection(&dev, 0);
		uint16_t xpos = 0;
		uint16_t ypos = 15;
		int xd = 0;
		int yd = 1;
		if(CONFIG_WIDTH < CONFIG_HEIGHT) {
			lcdSetFontDirection(&dev, 1);
			xpos = (CONFIG_WIDTH-1)-16;
			ypos = 0;
			xd = 1;
			yd = 0;
		}
		strcpy((char *)ascii, "16Dot Gothic Font");
		lcdDrawString(&dev, fx16G, xpos, ypos, ascii, color);

		xpos = xpos - (24 * xd) - (margin * xd);
		ypos = ypos + (16 * yd) + (margin * yd);
		strcpy((char *)ascii, "24Dot Gothic Font");
		lcdDrawString(&dev, fx24G, xpos, ypos, ascii, color);

		xpos = xpos - (32 * xd) - (margin * xd);
		ypos = ypos + (24 * yd) + (margin * yd);
		if (CONFIG_WIDTH >= 240) {
			strcpy((char *)ascii, "32Dot Gothic Font");
			lcdDrawString(&dev, fx32G, xpos, ypos, ascii, color);
			xpos = xpos - (32 * xd) - (margin * xd);;
			ypos = ypos + (32 * yd) + (margin * yd);
		}

		xpos = xpos - (10 * xd) - (margin * xd);
		ypos = ypos + (10 * yd) + (margin * yd);
		strcpy((char *)ascii, "16Dot Mincyo Font");
		lcdDrawString(&dev, fx16M, xpos, ypos, ascii, color);

		xpos = xpos - (24 * xd) - (margin * xd);;
		ypos = ypos + (16 * yd) + (margin * yd);
		strcpy((char *)ascii, "24Dot Mincyo Font");
		lcdDrawString(&dev, fx24M, xpos, ypos, ascii, color);

		if (CONFIG_WIDTH >= 240) {
			xpos = xpos - (32 * xd) - (margin * xd);;
			ypos = ypos + (24 * yd) + (margin * yd);
			strcpy((char *)ascii, "32Dot Mincyo Font");
			lcdDrawString(&dev, fx32M, xpos, ypos, ascii, color);
		}
		lcdDrawFinish(&dev);
		lcdSetFontDirection(&dev, 0);
		WAIT;

	} // end while

	// never reach here
	while (1) {
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

static void listSPIFFS(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

esp_err_t mountSPIFFS(char * path, char * label, int max_files) {
	esp_vfs_spiffs_conf_t conf = {
		.base_path = path,
		.partition_label = label,
		.max_files = max_files,
		.format_if_mount_failed = true
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret ==ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret== ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return ret;
	}

#if 0
	ESP_LOGI(TAG, "Performing SPIFFS_check().");
	ret = esp_spiffs_check(conf.partition_label);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
		return ret;
	} else {
			ESP_LOGI(TAG, "SPIFFS_check() successful");
	}
#endif

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(conf.partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Mount %s to %s success", path, label);
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}

	return ret;
}


void app_main(void)
{
	ESP_LOGI(TAG, "Initializing SPIFFS");
	// Maximum files that could be open at the same time is 7.
	ESP_ERROR_CHECK(mountSPIFFS("/fonts", "storage1", 7));
	listSPIFFS("/fonts/");

	// Maximum files that could be open at the same time is 1.
	ESP_ERROR_CHECK(mountSPIFFS("/images", "storage2", 1));
	listSPIFFS("/images/");

	// Maximum files that could be open at the same time is 1.
	ESP_ERROR_CHECK(mountSPIFFS("/icons", "storage3", 1));
	listSPIFFS("/icons/");

	xTaskCreate(ST7789, "ST7789", 1024*6, NULL, 2, NULL);
}
