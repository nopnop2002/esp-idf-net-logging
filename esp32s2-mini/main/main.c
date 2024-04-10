/* The example of ESP-IDF net-logging
 *
 * This sample code is in the public domain.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"

#include "lwip/init.h"
//#include "esp_spi_flash.h" ESP-IDF V4
#include "esp_flash.h" // ESP-IDF V5

#include "net_logging.h"

static const char *TAG = "MAIN";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void wifi_init_sta(void)
{
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&event_handler,
		NULL,
		&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&event_handler,
		NULL,
		&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_ESP_WIFI_SSID,
			.password = CONFIG_ESP_WIFI_PASSWORD,
			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			 * However these modes are deprecated and not advisable to be used. Incase your Access point
			 * doesn't support WPA2, these mode can be enabled by commenting below line */
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,

			.pmf_cfg = {
				.capable = true,
				.required = false
			},
		}, // end of sta
	}; // end of wifi_config
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG, "wifi_init_sta finished.");

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
		WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}

	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
}

int spi_flash_size() {
	uint32_t chip_id;
	ESP_ERROR_CHECK(esp_flash_read_id(NULL, &chip_id));
	ESP_LOGI(TAG, "chip ID=0x%"PRIx32, chip_id);
	int flash_capacity = chip_id & 0xff;
	ESP_LOGI(TAG, "flash_capacity=0x%x", flash_capacity);
	int real_flash_size = 0;
	if (flash_capacity == 0x15) {
		real_flash_size = 2;
	} else if (flash_capacity == 0x16) {
		real_flash_size = 4;
	} else if (flash_capacity == 0x17) {
		real_flash_size = 8;
	} else if (flash_capacity == 0x18) {
		real_flash_size = 16;
	}
	ESP_LOGD(TAG, "real_flash_siz=%d", real_flash_size);
	return real_flash_size;
}


void app_main()
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Initialize WiFi
	wifi_init_sta();

	int16_t write_to_stdout = 0;
#if CONFIG_WRITE_TO_STDOUT
	ESP_LOGI(TAG, "Enable write Logging to STDOUT");
	write_to_stdout = 1;
#endif

#if CONFIG_ENABLE_UDP_LOG
	ESP_ERROR_CHECK(udp_logging_init( CONFIG_LOG_UDP_SERVER_IP, CONFIG_LOG_UDP_SERVER_PORT, write_to_stdout ));
#endif // CONFIG_ENABLE_UDP_LOG

#if CONFIG_ENABLE_TCP_LOG
	ESP_ERROR_CHECK(tcp_logging_init( CONFIG_LOG_TCP_SERVER_IP, CONFIG_LOG_TCP_SERVER_PORT, write_to_stdout ));
#endif // CONFIG_ENABLE_TCP_LOG

#if CONFIG_ENABLE_MQTT_LOG
	ESP_ERROR_CHECK(mqtt_logging_init( CONFIG_LOG_MQTT_SERVER_URL, CONFIG_LOG_MQTT_PUB_TOPIC, write_to_stdout ));
#endif // CONFIG_ENABLE_MQTT_LOG

#if CONFIG_ENABLE_HTTP_LOG
	ESP_ERROR_CHECK(http_logging_init( CONFIG_LOG_HTTP_SERVER_URL, write_to_stdout ));
#endif // CONFIG_ENABLE_HTTP_LOG

	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	// Print out embedded or external
	uint32_t embedded = chip_info.features & CHIP_FEATURE_EMB_FLASH;
	ESP_LOGI(TAG, "embedded=%"PRIx32, embedded);
	if (embedded) {
		ESP_LOGI(TAG, "embedded flash");
		uint32_t flash_size;
		ESP_ERROR_CHECK(esp_flash_get_size(NULL, &flash_size));
		size_t flash_size_k =  flash_size / 1024;
		size_t flash_size_m =  flash_size_k / 1024;
		ESP_LOGI(TAG, "embedded %d MBytes flash", flash_size_m);
	} else {
		ESP_LOGI(TAG, "external flash");
		int flash_size = spi_flash_size();
		ESP_LOGI(TAG, "external %d Mbytes flash", flash_size);
	}

	size_t psram_size = esp_psram_get_size();
	size_t psram_size_k = psram_size/1024;
	size_t psram_size_m = psram_size_k/1024;
	ESP_LOGI(TAG, "PSRAM size: %d bytes %d Mbytes", psram_size, psram_size_m);
	ESP_LOGI(TAG, "MALLOC_CAP_SPIRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
	ESP_LOGI(TAG, "Free heap size: %"PRIu32, esp_get_free_heap_size());
	//ESP_LOGI(TAG, "Free heap size (caps): %u", heap_caps_get_free_size(MALLOC_CAP_8BIT));
}


