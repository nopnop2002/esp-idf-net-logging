#ifndef NET_LOGGING_H_
#define NET_LOGGING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"
#include "esp_log.h"

typedef struct {
	uint16_t port;
	char ipv4[20]; // xxx.xxx.xxx.xxx
	char url[64]; // mqtt://iot.eclipse.org
	char topic[64];
	TaskHandle_t taskHandle;
} PARAMETER_t;

// The total number of bytes (not messages) the message buffer will be able to hold at any one time.
#define xBufferSizeBytes 1024
// The size, in bytes, required to hold each item in the message,
#define xItemSize 256


int logging_vprintf( const char *fmt, va_list l );
esp_err_t udp_logging_init(char *ipaddr, unsigned long port, int16_t enableStdout);
esp_err_t tcp_logging_init(char *ipaddr, unsigned long port, int16_t enableStdout);
esp_err_t mqtt_logging_init(char *url, char *topic, int16_t enableStdout);
esp_err_t http_logging_init(char *url, int16_t enableStdout);

#ifdef __cplusplus
}
#endif

#endif /* NET_LOGGING_H_ */
