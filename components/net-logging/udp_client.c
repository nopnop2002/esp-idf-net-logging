/* UDP Client

	 This example code is in the Public Domain (or CC0 licensed, at your option.)

	 Unless required by applicable law or agreed to in writing, this
	 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	 CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/message_buffer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lwip/sockets.h"

#include "net_logging.h"

extern MessageBufferHandle_t xMessageBufferTrans;

void udp_dump(char *id, char *data, int len)
{
  int i;
  printf("[%s]\n",id);
  for(i=0;i<len;i++) {
    printf("%0x ",data[i]);
    if ( (i % 10) == 9) printf("\n");
  }
  printf("\n");
}

// UDP Client Task
void udp_client(void *pvParameters) {
	PARAMETER_t *task_parameter = pvParameters;
	PARAMETER_t param;
	memcpy((char *)&param, task_parameter, sizeof(PARAMETER_t));
	//printf("Start:param.port=%d param.ipv4=[%s]\n", param.port, param.ipv4);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(param.port);
	//addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); /* send message to 255.255.255.255 */
	//addr.sin_addr.s_addr = inet_addr("255.255.255.255"); /* send message to 255.255.255.255 */
	addr.sin_addr.s_addr = inet_addr(param.ipv4);

	/* create the socket */
	int fd;
	int ret;
	fd = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Create a UDP socket.
	LWIP_ASSERT("fd >= 0", fd >= 0);

	// Send ready to receive notify
	char buffer[xItemSize];
	xTaskNotifyGive(param.taskHandle);

	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferTrans, buffer, sizeof(buffer), portMAX_DELAY);
		//printf("xMessageBufferReceive received=%d\n", received);
		if (received > 0) {
			//printf("xMessageBufferReceive buffer=[%.*s]\n",received, buffer);
			//udp_dump("buffer", buffer, received);

/*
buffer included escape code
[buffer]
1b 5b 30 3b 33 32 6d 49 20 28
36 31 32 33 29 20 4d 41 49 4e
3a 20 63 68 69 70 20 6d 6f 64
65 6c 20 69 73 20 31 2c 20 1b
5b 30 6d a

ESC [ 0 ; 3 2 m I SP (
6 1 2 3 ) SP M A I N
: SP c h i p SO m o d
e l SP i s SP 1 , SP ESC
[ 0 m LF

I (6123) MAIN: chip model is 1,

Start coloring:
ESC [ 0 ; 3 2 m
ESC [ ColorCode m
Finished coloring:
ESC [ 0 m



buffer NOT included escape code
[buffer]
3c 62 61 2d 61 64 64 3e 69 64
78 3a 31 20 28 69 66 78 3a 30
2c 20 66 38 3a 62 37 3a 39 37
3a 33 36 3a 64 65 3a 35 32 29
2c 20 74 69 64 3a 30 2c 20 73
73 6e 3a 34 2c 20 77 69 6e 53
69 7a 65 3a 36 34
*/

			ret = lwip_sendto(fd, buffer, received, 0, (struct sockaddr *)&addr, sizeof(addr));
			LWIP_ASSERT("ret == received", ret == received);
		} else {
			printf("xMessageBufferReceive fail\n");
			break;
		}
	} // end while

	// Close socket
	ret = lwip_close(fd);
	LWIP_ASSERT("ret == 0", ret == 0);
	vTaskDelete( NULL );

}

