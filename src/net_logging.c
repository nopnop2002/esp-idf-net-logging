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
RingbufHandle_t xRingBufferUDP = NULL;
RingbufHandle_t xRingBufferMulticast = NULL;
RingbufHandle_t xRingBufferTCP = NULL;
RingbufHandle_t xRingBufferMQTT = NULL;
RingbufHandle_t xRingBufferHTTP = NULL;
RingbufHandle_t xRingBufferSSE = NULL;
//RingbufHandle_t xRingBufferTrans;
#else
MessageBufferHandle_t xMessageBufferUDP = NULL;
MessageBufferHandle_t xMessageBufferMulticast = NULL;
MessageBufferHandle_t xMessageBufferTCP = NULL;
MessageBufferHandle_t xMessageBufferMQTT = NULL;
MessageBufferHandle_t xMessageBufferHTTP = NULL;
MessageBufferHandle_t xMessageBufferSSE = NULL;
//MessageBufferHandle_t xMessageBufferTrans;
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
		BaseType_t sended;
		if (xRingBufferUDP != NULL) {
			sended = xRingbufferSendFromISR(xRingBufferUDP, buffer, strlen(buffer), &xHigherPriorityTaskWoken);
			(void)sended; //assert(sended == pdTRUE); -- don't die if buffer overflows
		}
        if (xRingBufferMulticast != NULL) {
            sended = xRingbufferSendFromISR(xRingBufferMulticast, buffer, strlen(buffer), &xHigherPriorityTaskWoken);
            (void)sended; //assert(sended == pdTRUE); -- don't die if buffer overflows
        }
		if (xRingBufferTCP != NULL) {
			sended = xRingbufferSendFromISR(xRingBufferTCP, buffer, cstr_len, &xHigherPriorityTaskWoken);
			(void)sended; //assert(sended == pdTRUE); -- don't die if buffer overflows
		}
		if (xRingBufferMQTT != NULL) {
			sended = xRingbufferSendFromISR(xRingBufferMQTT, buffer, strlen(buffer), &xHigherPriorityTaskWoken);
			(void)sended; //assert(sended == pdTRUE); -- don't die if buffer overflows
		}
		if (xRingBufferHTTP != NULL) {
			sended = xRingbufferSendFromISR(xRingBufferHTTP, buffer, strlen(buffer), &xHigherPriorityTaskWoken);
			(void)sended; //assert(sended == pdTRUE); -- don't die if buffer overflows
		}
		if (xRingBufferSSE != NULL) {
			sended = xRingbufferSendFromISR(xRingBufferSSE, buffer, strlen(buffer), &xHigherPriorityTaskWoken);
			(void)sended; //assert(sended == pdTRUE); -- don't die if buffer overflows
		}
#else
		// Send MessageBuffer
		size_t sended;
		if (xMessageBufferUDP != NULL) {
			sended = xMessageBufferSendFromISR(xMessageBufferUDP, buffer, buffer_len, &xHigherPriorityTaskWoken);
			// assert(sended == str_len); -- don't die if buffer overflows
		}
        if (xMessageBufferMulticast != NULL) {
            sended = xMessageBufferSendFromISR(xMessageBufferMulticast, buffer, buffer_len, &xHigherPriorityTaskWoken);
            // assert(sended == str_len); -- don't die if buffer overflows
        }
		if (xMessageBufferTCP != NULL) {
			sended = xMessageBufferSendFromISR(xMessageBufferTCP, buffer, cstr_len, &xHigherPriorityTaskWoken);
			// assert(sended == str_len); -- don't die if buffer overflows
		}
		if (xMessageBufferMQTT != NULL) {
			sended = xMessageBufferSendFromISR(xMessageBufferMQTT, buffer, buffer_len, &xHigherPriorityTaskWoken);
			// assert(sended == str_len); -- don't die if buffer overflows
		}
		if (xMessageBufferHTTP != NULL) {
			sended = xMessageBufferSendFromISR(xMessageBufferHTTP, buffer, buffer_len, &xHigherPriorityTaskWoken);
			// assert(sended == str_len); -- don't die if buffer overflows
		}
		if (xMessageBufferSSE != NULL) {
			sended = xMessageBufferSendFromISR(xMessageBufferSSE, buffer, buffer_len, &xHigherPriorityTaskWoken);
			// assert(sended == str_len); -- don't die if buffer overflows
		}
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

esp_err_t udp_logging_init(const char *ipaddr, unsigned long port, bool enableStdout) {

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
	printf("start udp logging(xRingBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create RineBuffer
	xRingBufferUDP = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferUDP );
#else
	printf("start udp logging(xMessageBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create MessageBuffer
	xMessageBufferUDP = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferUDP );
#endif

	// Start UDP task
	PARAMETER_t param;
	param.port = port;
	strcpy(param.ipv4, ipaddr);
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(udp_client, "UDP", 1024*6, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	uint32_t value = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(1000) );
	printf("udp ulTaskNotifyTake=%"PRIi32"\n", value);
	if (value == 0) {
		printf("stop udp logging\n");
#if CONFIG_NET_LOGGING_USE_RINGBUFFER
		vRingbufferDelete(xRingBufferUDP);
		xRingBufferUDP = NULL;
#else
		vMessageBufferDelete(xMessageBufferUDP);
		xMessageBufferUDP = NULL;
#endif
    }

    // Set function used to output log entries.
    writeToStdout = enableStdout;
    esp_log_set_vprintf(logging_vprintf);
    return ESP_OK;
}

void multicast_log_sender(void *pvParameters);

esp_err_t multicast_log_sender_init(const char *ipaddr, unsigned long port, bool enableStdout) {

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
	printf("start multicast logging(xRingBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create RineBuffer
	xRingBufferMulticast = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferMulticast );
#else
	printf("start multicast logging(xMessageBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create MessageBuffer
	xMessageBufferMulticast = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferMulticast );
#endif
	
    // Allocate memory for the parameter structure
    // udp_client() will free it when done
	PARAMETER_t *param = malloc(sizeof(PARAMETER_t)); 
    if (param == NULL) {
        printf("%s: malloc fail\n", __func__);
        return ESP_ERR_NO_MEM;
    }
	param->port = port;
	strcpy(param->ipv4, ipaddr);
	param->taskHandle = xTaskGetCurrentTaskHandle();
    // Start Multicast Sender task
	xTaskCreate(multicast_log_sender, "MCAST", 1024*6, (void *)param, 2, NULL);

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}

void tcp_client(void *pvParameters);

esp_err_t tcp_logging_init(const char *ipaddr, unsigned long port, bool enableStdout) {

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
	printf("start tcp logging(xRingBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create RineBuffer
	xRingBufferTCP = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferTCP );
#else
	printf("start tcp logging(xMessageBuffer): ipaddr=[%s] port=%ld\n", ipaddr, port);
	// Create MessageBuffer
	xMessageBufferTCP = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferTCP );
#endif

	// Start TCP task
	PARAMETER_t param;
	param.port = port;
	strcpy(param.ipv4, ipaddr);
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(tcp_client, "TCP", 1024*6, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	uint32_t value = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(1000) );
	printf("tcp ulTaskNotifyTake=%"PRIi32"\n", value);
	if (value == 0) {
		printf("stop tcp logging\n");
#if CONFIG_NET_LOGGING_USE_RINGBUFFER
		vRingbufferDelete(xRingBufferTCP);
		xRingBufferTCP = NULL;
#else
		vMessageBufferDelete(xMessageBufferTCP);
		xMessageBufferTCP = NULL;
#endif
	}

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}

void sse_server(void *pvParameters);

esp_err_t sse_logging_init(unsigned long port, bool enableStdout) {

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
	printf("start HTTP Server Sent Events logging(xRingBuffer): SSE server listening on port=%ld\n", port);
	// Create RineBuffer
	xRingBufferSSE = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferSSE );
#else
	printf("start HTTP Server Sent Events logging(xMessageBuffer): SSE server starting on port=%ld\n", port);
	// Create MessageBuffer
	xMessageBufferSSE = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferSSE );
#endif

	// Start SSE Server
	PARAMETER_t param;
	param.port = port;
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(sse_server, "HTTP SSE", 1024*6, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	uint32_t value = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(1000) );
	printf("sse ulTaskNotifyTake=%"PRIi32"\n", value);
	if (value == 0) {
		printf("stop HTTP SSE logging\n");
#if CONFIG_NET_LOGGING_USE_RINGBUFFER
		vRingbufferDelete(xRingBufferSSE);
		xRingBufferSSE = NULL;
#else
		vMessageBufferDelete(xMessageBufferSSE);
		xMessageBufferSSE = NULL;
#endif
	}

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}

void mqtt_pub(void *pvParameters);

esp_err_t mqtt_logging_init(const char *url, char *topic, bool enableStdout) {

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
	printf("start mqtt logging(xRingBuffer): url=[%s] topic=[%s]\n", url, topic);
	// Create RineBuffer
	xRingBufferMQTT = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferMQTT );
#else
	printf("start mqtt logging(xMessageBuffer): url=[%s] topic=[%s]\n", url, topic);
	// Create MessageBuffer
	xMessageBufferMQTT = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferMQTT );
#endif

	// Start MQTT task
	PARAMETER_t param;
	strcpy(param.url, url);
	strcpy(param.topic, topic);
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(mqtt_pub, "MQTT", 1024*6, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	uint32_t value = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(1000) );
	printf("mqtt ulTaskNotifyTake=%"PRIi32"\n", value);
	if (value == 0) {
		printf("stop mqtt logging\n");
#if CONFIG_NET_LOGGING_USE_RINGBUFFER
		vRingbufferDelete(xRingBufferMQTT);
		xRingBufferMQTT = NULL;
#else
		vMessageBufferDelete(xMessageBufferMQTT);
		xMessageBufferMQTT = NULL;
#endif
	}

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}

void http_client(void *pvParameters);

esp_err_t http_logging_init(const char *url, bool enableStdout) {

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
	printf("start http logging(xRingBuffer): url=[%s]\n", url);
	// Create RineBuffer
	xRingBufferHTTP = xRingbufferCreate(xBufferSizeBytes, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingBufferHTTP );
#else
	printf("start http logging(xMessageBuffer): url=[%s]\n", url);
	// Create MessageBuffer
	xMessageBufferHTTP = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferHTTP );
#endif

	// Start HTTP task
	PARAMETER_t param;
	strcpy(param.url, url);
	param.taskHandle = xTaskGetCurrentTaskHandle();
	xTaskCreate(http_client, "HTTP", 1024*4, (void *)&param, 2, NULL);

	// Wait for ready to receive notify
	uint32_t value = ulTaskNotifyTake( pdTRUE, pdMS_TO_TICKS(1000) );
	printf("http ulTaskNotifyTake=%"PRIi32"\n", value);
	if (value == 0) {
		printf("stop http logging\n");
#if CONFIG_NET_LOGGING_USE_RINGBUFFER
		vRingbufferDelete(xRingBufferHTTP);
		xRingBufferHTTP = NULL;
#else
		vMessageBufferDelete(xMessageBufferHTTP);
		xMessageBufferHTTP = NULL;
#endif
	}

	// Set function used to output log entries.
	writeToStdout = enableStdout;
	esp_log_set_vprintf(logging_vprintf);
	return ESP_OK;
}
