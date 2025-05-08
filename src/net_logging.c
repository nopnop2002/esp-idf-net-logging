#include "net_logging.h"

#include <string.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#if CONFIG_NET_LOGGING_USE_RINGBUFFER
#include "freertos/ringbuf.h"
#else
#include "freertos/message_buffer.h"
#endif

#include "esp_system.h"
#include "esp_log.h"
#include "net_logging_priv.h"

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
RingbufHandle_t xRingBufferTrans;
#else
MessageBufferHandle_t xMessageBufferTrans;
#endif
bool writeToStdout;

static int logging_vprintf( const char *fmt, va_list l ) {
	// Convert according to format
	char *buffer = malloc(xItemSize);
    if (buffer == NULL) {
        printf("logging_vprintf malloc fail\n");
        return 0;
    }
	int len = vsnprintf(buffer, xItemSize, fmt, l);
    if (len >= xItemSize) {
        // Truncate the string to fit the buffer
        len = xItemSize - 1;
        buffer[len] = '\0'; // Ensure null-termination
    }
	//printf("logging_vprintf str_len=%d\n",str_len);
	//printf("logging_vprintf buffer=[%.*s]\n", str_len, buffer);
	if (len > 0) {
        const int cstr_len = len + 1; // Include null terminator
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#if CONFIG_NET_LOGGING_USE_RINGBUFFER
		// Send RingBuffer
		BaseType_t sended = xRingbufferSendFromISR(xRingBufferTrans, buffer, cstr_len, &xHigherPriorityTaskWoken);
		//printf("logging_vprintf sended=%d\n",sended);
		//assert(sended == pdTRUE); -- don't die if buffer overflows
#else
		// Send MessageBuffer
		size_t sended = xMessageBufferSendFromISR(xMessageBufferTrans, buffer, cstr_len, &xHigherPriorityTaskWoken);
		//printf("logging_vprintf sended=%d\n",sended);
		// assert(sended == str_len); -- don't die if buffer overflows
#endif

        // Write to stdout
        if (writeToStdout) {
            //return vprintf( fmt, l );
            //printf( "%s", buffer ); // we already formatted the string, so just print it
            fwrite( buffer, sizeof(char), cstr_len, stdout );
            //fflush(stdout); // make sure it gets printed immediately
        } 
	}

    free(buffer);
    return 0;
}

void udp_client(void *pvParameters);

esp_err_t udp_logging_init(const char *ipaddr, unsigned long port, int16_t enableStdout) {

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
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

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
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

void sse_server(void *pvParameters);

esp_err_t sse_logging_init(unsigned long port, int16_t enableStdout) {

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
	printf("start HTTP Server Sent Events logging(xRingBuffer): SSE server listening on port=%ld\n", port);
	// Create RineBuffer
	xRingBufferTrans = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferTrans );
#else
	printf("start HTTP Server Sent Events logging(xMessageBuffer): SSE server starting on port=%ld\n", port);
	// Create MessageBuffer
	xMessageBufferTrans = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferTrans );
#endif

	// Start SSE Server
	PARAMETER_t param;
	param.port = port;
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(sse_server, "HTTP SSE", 1024*6, (void *)&param, 2, NULL);

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

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
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

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
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
