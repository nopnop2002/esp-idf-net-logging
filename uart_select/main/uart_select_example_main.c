/* UART Select Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "driver/uart_vfs.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "net_logging.h"


static const char* TAG = "MAIN";

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

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

esp_err_t wifi_init_sta(void)
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
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	esp_err_t ret_value = ESP_OK;
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
		ret_value = ESP_FAIL;
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
		ret_value = ESP_FAIL;
	}

	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
	return ret_value;
}

static void uart_select_task(void *arg)
{
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};
	uart_driver_install(UART_NUM_0, 2*1024, 0, 0, NULL, 0);
	uart_param_config(UART_NUM_0, &uart_config);
	uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	while (1) {
		int fd;

		if ((fd = open("/dev/uart/0", O_RDWR)) == -1) {
			ESP_LOGE(TAG, "Cannot open UART");
			vTaskDelay(5000 / portTICK_PERIOD_MS);
			continue;
		}

		// We have a driver now installed so set up the read/write functions to use driver also.
		//esp_vfs_dev_uart_use_driver(0);
		uart_vfs_dev_use_driver(UART_NUM_0);

		while (1) {
			int s;
			fd_set rfds;
			struct timeval tv = {
				.tv_sec = 5,
				.tv_usec = 0,
			};

			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);

			s = select(fd + 1, &rfds, NULL, NULL, &tv);

			if (s < 0) {
				ESP_LOGE(TAG, "Select failed: errno %d", errno);
				break;
			} else if (s == 0) {
				ESP_LOGI(TAG, "Timeout has been reached and nothing has been received");
			} else {
				if (FD_ISSET(fd, &rfds)) {
					char buf;
					if (read(fd, &buf, 1) > 0) {
						ESP_LOGI(TAG, "Received: %c", buf);
						// Note: Only one character was read even the buffer contains more. The other characters will
						// be read one-by-one by subsequent calls to select() which will then return immediately
						// without timeout.
					} else {
						ESP_LOGE(TAG, "UART read error");
						break;
					}
				} else {
					ESP_LOGE(TAG, "No FD has been set in select()");
					break;
				}
			}
		}

		close(fd);
	}

	vTaskDelete(NULL);
}

void app_main(void)
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Initialize WiFi
	ESP_ERROR_CHECK(wifi_init_sta());

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

#if CONFIG_ENABLE_SSE_SERVER_LOG
	ESP_ERROR_CHECK(sse_logging_init( CONFIG_LOG_SSE_LISTEN_PORT, write_to_stdout ));
#endif // CONFIG_ENABLE_SSE_SERVER_LOG

	xTaskCreate(uart_select_task, "uart_select_task", 4*1024, NULL, 5, NULL);
}
