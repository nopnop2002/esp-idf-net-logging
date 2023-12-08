/*
	TCP Client

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
#include "esp_system.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "netdb.h" // gethostbyname

#include "net_logging.h"

#if CONFIG_USE_RINGBUFFER
extern RingbufHandle_t xRingBufferTrans;
#else
extern MessageBufferHandle_t xMessageBufferTrans;
#endif

void tcp_client(void *pvParameters)
{
	PARAMETER_t *task_parameter = pvParameters;
	PARAMETER_t param;
	memcpy((char *)&param, task_parameter, sizeof(PARAMETER_t));
	printf("Start:param.port=%d param.ipv4=[%s]\n", param.port, param.ipv4);

	int addr_family = 0;
	int ip_protocol = 0;

	struct sockaddr_in dest_addr;
	dest_addr.sin_addr.s_addr = inet_addr(param.ipv4);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(param.port);
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;

	printf("dest_addr.sin_addr.s_addr=0x%"PRIx32"\n", dest_addr.sin_addr.s_addr);
	if (dest_addr.sin_addr.s_addr == 0xffffffff) {
		struct hostent *hp;
		hp = gethostbyname(param.ipv4);
		if (hp == NULL) {
			printf("FTP Client Error: Connect, gethostbyname\n");
			vTaskDelete(NULL);
		}
		struct ip4_addr *ip4_addr;
		ip4_addr = (struct ip4_addr *)hp->h_addr;
		dest_addr.sin_addr.s_addr = ip4_addr->addr;
		printf("dest_addr.sin_addr.s_addr=0x%"PRIx32"\n", dest_addr.sin_addr.s_addr);
	}
	

	int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
	if (sock < 0) {
		//ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		vTaskDelete(NULL);
	}
	printf("Socket created, connecting to %s:%d\n", param.ipv4, param.port);

	int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
	if (err == 0) {
		printf("Successfully connected\n");
	} else {
		printf("Socket unable to connect: errno %d\n", errno);
		vTaskDelete(NULL);
	}

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
			int ret = send(sock, buffer, received, 0);
			LWIP_ASSERT("ret == received", ret == received);
#if CONFIG_USE_RINGBUFFER
            vRingbufferReturnItem(xRingBufferTrans, (void *)buffer);
#endif
		} else {
			//printf("xMessageBufferReceive fail\n");
			break;
		} // end if
	} // end while

	if (sock != -1) {
		//ESP_LOGE(TAG, "Shutting down socket and restarting...");
		shutdown(sock, 0);
		close(sock);
	}
	vTaskDelete(NULL);
}
