#ifndef NET_LOGGING_PRIV_H_
#define NET_LOGGING_PRIV_H_

#include <stdint.h>
#include "freertos/FreeRTOS.h" // for TaskHandle_t

#include "net_logging.h"

// The total number of bytes (not messages) the message buffer will be able to hold at any one time.
#define xBufferSizeBytes (CONFIG_NET_LOGGING_BUFFER_SIZE) // set this in menuconfig

// The size, in bytes, required to hold each item in the message,
#define xItemSize (CONFIG_NET_LOGGING_MESSAGE_MAX_LENGTH) // set this in menuconfig

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint16_t port;
	char ipv4[20]; // xxx.xxx.xxx.xxx
	char url[64]; // mqtt://iot.eclipse.org
	char topic[64];
	TaskHandle_t taskHandle;
} PARAMETER_t;


#ifdef __cplusplus
}
#endif

#endif /* NET_LOGGING_PRIV_H_ */
