#include <string.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#if CONFIG_USE_RINGBUFFER
#include "freertos/ringbuf.h"
#else
#include "freertos/message_buffer.h"
#endif

#include "esp_system.h"
#include "esp_log.h"

#include "net_logging.h"

#if CONFIG_USE_RINGBUFFER
RingbufHandle_t xRingBufferTrans;
#else
MessageBufferHandle_t xMessageBufferTrans;
#endif
bool writeToStdout;

int logging_vprintf( const char *fmt, va_list l ) {
	// Convert according to format
	char buffer[xItemSize];
	int buffer_len = vsprintf(buffer, fmt, l);
	//printf("logging_vprintf buffer_len=%d\n",buffer_len);
	//printf("logging_vprintf buffer=[%.*s]\n", buffer_len, buffer);
	if (buffer_len > 0) {
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#if CONFIG_USE_RINGBUFFER
		// Send RingBuffer
		BaseType_t sended = xRingbufferSendFromISR(xRingBufferTrans, &buffer, strlen(buffer), &xHigherPriorityTaskWoken);
		//printf("logging_vprintf sended=%d\n",sended);
		assert(sended == pdTRUE);
#else
		// Send MessageBuffer
		size_t sended = xMessageBufferSendFromISR(xMessageBufferTrans, &buffer, buffer_len, &xHigherPriorityTaskWoken);
		//printf("logging_vprintf sended=%d\n",sended);
		assert(sended == buffer_len);
#endif
	}

	// Write to stdout
	if (writeToStdout) {
		return vprintf( fmt, l );
	} else {
		return 0;
	}
}

void udp_client(void *pvParameters);

esp_err_t udp_logging_init(const char *ipaddr, unsigned long port, int16_t enableStdout) {

#if CONFIG_USE_RINGBUFFER
	printf("start udp logging(xRingBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create RineBuffer
	xRingBufferTrans = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferTrans );
#else
	printf("start udp logging(xMessageBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create MessageBuffer
	xMessageBufferTrans = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferTrans );
#endif

	// Start UDP task
	PARAMETER_t param;
	param.port = port;
	strcpy(param.ipv4, ipaddr);
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(udp_client, "UDP", 1024*6, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
	//printf("ulTaskNotifyTake\n");

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}

void tcp_client(void *pvParameters);

esp_err_t tcp_logging_init(const char *ipaddr, unsigned long port, int16_t enableStdout) {

#if CONFIG_USE_RINGBUFFER
	printf("start tcp logging(xRingBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create RineBuffer
	xRingBufferTrans = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferTrans );
#else
	printf("start tcp logging(xMessageBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create MessageBuffer
	xMessageBufferTrans = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferTrans );
#endif

	// Start TCP task
	PARAMETER_t param;
	param.port = port;
	strcpy(param.ipv4, ipaddr);
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(tcp_client, "TCP", 1024*6, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
	//printf("ulTaskNotifyTake\n");

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}

void mqtt_pub(void *pvParameters);

esp_err_t mqtt_logging_init(const char *url, char *topic, int16_t enableStdout) {

#if CONFIG_USE_RINGBUFFER
	printf("start mqtt logging(xRingBuffer): url=[%s] topic=[%s]\n", url, topic);
	// Create RineBuffer
	xRingBufferTrans = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferTrans );
#else
	printf("start mqtt logging(xMessageBuffer): url=[%s] topic=[%s]\n", url, topic);
	// Create MessageBuffer
	xMessageBufferTrans = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferTrans );
#endif

	// Start MQTT task
	PARAMETER_t param;
	strcpy(param.url, url);
	strcpy(param.topic, topic);
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(mqtt_pub, "MQTT", 1024*6, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
	//printf("ulTaskNotifyTake\n");

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}

void http_client(void *pvParameters);

esp_err_t http_logging_init(const char *url, int16_t enableStdout) {

#if CONFIG_USE_RINGBUFFER
	printf("start http logging(xRingBuffer): url=[%s]\n", url);
	// Create RineBuffer
	xRingBufferTrans = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferTrans );
#else
	printf("start http logging(xMessageBuffer): url=[%s]\n", url);
	// Create MessageBuffer
	xMessageBufferTrans = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferTrans );
#endif

	// Start HTTP task
	PARAMETER_t param;
	strcpy(param.url, url);
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(http_client, "HTTP", 1024*4, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
	//printf("ulTaskNotifyTake\n");

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}
