/*
	MQTT over TCP

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#if CONFIG_USE_RINGBUFFER
#include "freertos/ringbuf.h"
#else
#include "freertos/message_buffer.h"
#endif
#include "esp_log.h"
#include "esp_event.h"
#include "esp_mac.h" // esp_base_mac_addr_get
#include "mqtt_client.h"

#include "net_logging.h"

EventGroupHandle_t mqtt_status_event_group;
#define MQTT_CONNECTED_BIT BIT2

#if CONFIG_USE_RINGBUFFER
extern RingbufHandle_t xRingBufferTrans;
#else
extern MessageBufferHandle_t xMessageBufferTrans;
#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
#else
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
#endif
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
	esp_mqtt_event_handle_t event = event_data;
#endif
	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED:
			//ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			xEventGroupSetBits(mqtt_status_event_group, MQTT_CONNECTED_BIT);
			break;
		case MQTT_EVENT_DISCONNECTED:
			//ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			xEventGroupClearBits(mqtt_status_event_group, MQTT_CONNECTED_BIT);
			break;
		case MQTT_EVENT_SUBSCRIBED:
			//ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			//ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			//ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_DATA:
			//ESP_LOGI(TAG, "MQTT_EVENT_DATA");
			break;
		case MQTT_EVENT_ERROR:
			//ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
			break;
		default:
			//ESP_LOGI(TAG, "Other event id:%d", event->event_id);
			break;
	}
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
	return ESP_OK;
#endif
}

void mqtt_pub(void *pvParameters)
{
	PARAMETER_t *task_parameter = pvParameters;
	PARAMETER_t param;
	memcpy((char *)&param, task_parameter, sizeof(PARAMETER_t));
	//printf("Start:param.url=[%s] param.topic=[%s]\n", param.url, param.topic);

	// Create Event Group
	mqtt_status_event_group = xEventGroupCreate();
	configASSERT( mqtt_status_event_group );
	
	// Set client id from mac
	uint8_t mac[8];
	ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));
	char client_id[64];
	sprintf(client_id, "pub-%02x%02x%02x%02x%02x%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	//printf("client_id=[%s]\n", client_id);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = param.url,
		.credentials.client_id = client_id
	};
#else
	esp_mqtt_client_config_t mqtt_cfg = {
		.uri = param.url,
		.event_handle = mqtt_event_handler,
		.client_id = client_id
	};
#endif

	// Connect broker
	esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
	esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
#endif

	esp_mqtt_client_start(mqtt_client);
	xEventGroupClearBits(mqtt_status_event_group, MQTT_CONNECTED_BIT);

	// Wait for connection
	xEventGroupWaitBits(mqtt_status_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);
	//printf("Connected to MQTT Broker\n");

	// Send ready to receive notify
	xTaskNotifyGive(param.taskHandle);

	while (1) {
#if CONFIG_USE_RINGBUFFER
		size_t received;
		char *buffer = (char *)xRingbufferReceive(xRingBufferTrans, &received, portMAX_DELAY);
		//printf("xRingBufferReceive received=%d\n", received);
#else
		char buffer[xItemSize];
		size_t received = xMessageBufferReceive(xMessageBufferTrans, buffer, sizeof(buffer), portMAX_DELAY);
		//printf("xMessageBufferReceive received=%d\n", received);
#endif
		if (received > 0) {
			//printf("xMessageBufferReceive buffer=[%.*s]\n",received, buffer);
			EventBits_t EventBits = xEventGroupGetBits(mqtt_status_event_group);
			//printf("EventBits=%x\n", EventBits);
			if (EventBits & MQTT_CONNECTED_BIT) {
				// Remove trailing LF
				if (buffer[received-1] == 0x0a) received = received - 1;
				if (received) {
					esp_mqtt_client_publish(mqtt_client, param.topic, buffer, received, 1, 0);
					//printf("sent publish successful\n");
				}
			} else {
				printf("Connection to MQTT broker is broken. Skip to send\n");
			}
#if CONFIG_USE_RINGBUFFER
			vRingbufferReturnItem(xRingBufferTrans, (void *)buffer);
#endif
		} else {
			printf("xMessageBufferReceive fail\n");
			break;
		}
	} // end while

	// Stop connection
	esp_mqtt_client_stop(mqtt_client);
	vTaskDelete(NULL);

}
