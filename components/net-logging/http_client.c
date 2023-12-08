/*
	HTTP POST

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#if CONFIG_USE_RINGBUFFER
#include "freertos/ringbuf.h"
#else
#include "freertos/message_buffer.h"
#endif
#include "esp_log.h"
#include "esp_system.h"
#include "esp_tls.h"
#include "esp_http_client.h"

#include "net_logging.h"

#if CONFIG_USE_RINGBUFFER
extern RingbufHandle_t xRingBufferTrans;
#else
extern MessageBufferHandle_t xMessageBufferTrans;
#endif

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	static char *output_buffer;  // Buffer to store response of http request from event handler
	static int output_len;		 // Stores number of bytes read
	int mbedtls_err = 0;
	esp_err_t err;
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			//ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			//ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			//ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			//ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			//ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			/*
			 *	Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
			 *	However, event handler can also be used in case chunked encoding is used.
			 */
			if (!esp_http_client_is_chunked_response(evt->client)) {
				// If user_data buffer is configured, copy the response into the buffer
				if (evt->user_data) {
					memcpy(evt->user_data + output_len, evt->data, evt->data_len);
				} else {
					if (output_buffer == NULL) {
						output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
						output_len = 0;
						if (output_buffer == NULL) {
							//ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
							return ESP_FAIL;
						}
					}
					memcpy(output_buffer + output_len, evt->data, evt->data_len);
				}
				output_len += evt->data_len;
			}

			break;
		case HTTP_EVENT_ON_FINISH:
			//ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			if (output_buffer != NULL) {
				// Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
				// ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_DISCONNECTED:
			//ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
			//int mbedtls_err = 0;
			//esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
			err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
			if (err != 0) {
				if (output_buffer != NULL) {
					free(output_buffer);
					output_buffer = NULL;
				}
				output_len = 0;
				//ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
				//ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
			}
			break;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
		case HTTP_EVENT_REDIRECT:
			//ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
			break;
#endif
	}
	return ESP_OK;
}

#define MAX_HTTP_OUTPUT_BUFFER 128

static void http_post_with_url(char *url, char * post_data, size_t post_len)
{
	//ESP_LOGI(TAG, "http_post_with_url url=[%s]", url);
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	/**
	 * NOTE: All the configuration parameters for http_client must be spefied either in URL or as host and path parameters.
	 * If host and path parameters are not set, query parameter will be ignored. In such cases,
	 * query parameter should be specified in URL.
	 *
	 * If URL as well as host and path parameters are specified, values of host and path will be considered.
	 */

#if 1
	esp_http_client_config_t config = {
		.url = url,
		.path = "/post",
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer,			 // Pass address of local buffer to get response
		.disable_auto_redirect = true,
	};
#endif

#if 0
	esp_http_client_config_t config = {
		.url = "http://192.168.10.46:8000",
		.path = "/post",
		.event_handler = _http_event_handler,
		.user_data = local_response_buffer,			 // Pass address of local buffer to get response
		.disable_auto_redirect = true,
	};
#endif


	esp_http_client_handle_t client = esp_http_client_init(&config);

	// POST
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/json");
	//esp_http_client_set_post_field(client, post_data, strlen(post_data));
	esp_http_client_set_post_field(client, post_data, post_len);
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
#if 0
		ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
			esp_http_client_get_status_code(client),
			esp_http_client_get_content_length(client));
		ESP_LOGI(TAG, "local_response_buffer=[%s]", local_response_buffer);
#endif
	} else {
		printf("HTTP POST request failed: %s", esp_err_to_name(err));
	}

	esp_http_client_cleanup(client);
}


void http_client(void *pvParameters)
{
	PARAMETER_t *task_parameter = pvParameters;
	PARAMETER_t param;
	memcpy((char *)&param, task_parameter, sizeof(PARAMETER_t));
	//printf("Start:param.url=[%s]\n", param.url);

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
			// Remove trailing LF
			if (buffer[received-1] == 0x0a) received = received - 1;
			if (received) {
				http_post_with_url(param.url, buffer, received);
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
	vTaskDelete(NULL);
}
