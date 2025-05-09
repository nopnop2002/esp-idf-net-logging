#ifndef NET_LOGGING_H_
#define NET_LOGGING_H_

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t udp_logging_init(const char *ipaddr, unsigned long port, bool enableStdout);
esp_err_t multicast_log_sender_init(const char *ipaddr, unsigned long port, bool enableStdout);
esp_err_t tcp_logging_init(const char *ipaddr, unsigned long port, bool enableStdout);
esp_err_t mqtt_logging_init(const char *url, char *topic, bool enableStdout);
esp_err_t http_logging_init(const char *url, bool enableStdout);
esp_err_t sse_logging_init(unsigned long port, bool enableStdout);

#ifdef __cplusplus
}
#endif

#endif /* NET_LOGGING_H_ */
