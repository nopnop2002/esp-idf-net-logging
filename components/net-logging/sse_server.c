/*
  SSE server

  This example code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> // for close()

#include "freertos/FreeRTOS.h"
#include "freertos/task.h" // for vTaskDelete()
#if CONFIG_USE_RINGBUFFER
#include "freertos/ringbuf.h"
#else
#include "freertos/message_buffer.h"
#endif
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "net_logging.h"

// File content buffer for sse.html
extern const unsigned char sse_html_start[] asm("_binary_sse_html_start");
extern const unsigned char sse_html_end[] asm("_binary_sse_html_end");

#if CONFIG_USE_RINGBUFFER
extern RingbufHandle_t xRingBufferTrans;
#else
extern MessageBufferHandle_t xMessageBufferTrans;
#endif

void serve_client(int client_sock) {
  const size_t sse_html_size = sse_html_end - sse_html_start;

  // Receive HTTP request
  char rx_buffer[1024];
  int rx_len = recv(client_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
  if (rx_len <= 0) {
    // printf("Connection closed or error\n");
    close(client_sock);
    return;
  }

  // Null-terminate received data
  rx_buffer[rx_len] = 0;

  // Check if the request is for the root path (/)
  if (strstr(rx_buffer, "GET / HTTP") != NULL) {
    // Serve the HTML file
    // printf("Serving HTML file\n");

    // Prepare HTTP response
    char resp_header[256];
    snprintf(resp_header, sizeof(resp_header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n",
             sse_html_size);

    // Send header
    send(client_sock, resp_header, strlen(resp_header), 0);

    // Send file content
    send(client_sock, sse_html_start, sse_html_size, 0);
  }
  // Check if the request is for the SSE endpoint
  else if (strstr(rx_buffer, "GET /log-events HTTP") != NULL) {
    // Set up SSE connection
    // printf("SSE connection established\n");

    // Send SSE headers
    const char *sse_headers = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/event-stream\r\n"
                              "Cache-Control: no-cache\r\n"
                              "Connection: keep-alive\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "\r\n";

    send(client_sock, sse_headers, strlen(sse_headers), 0);

    // Keep connection open and send SSE events
    while (1) {
      #if CONFIG_USE_RINGBUFFER
      size_t received;
      char *buffer = (char *)xRingbufferReceive(xRingBufferTrans, &received, portMAX_DELAY);
      #else
      char buffer[xItemSize];
      size_t received = xMessageBufferReceive(xMessageBufferTrans, buffer, sizeof(buffer), portMAX_DELAY);
      #endif

      if (received > 0) {
        // Format the buffer content as an SSE event
        char sse_event[512];
        snprintf(sse_event, sizeof(sse_event), "event: log-line\ndata: %.*s\n\n", (int)received, buffer);

        #if CONFIG_USE_RINGBUFFER
        vRingbufferReturnItem(xRingBufferTrans, (void *)buffer);
        #endif

        // Send the event
        int ret = send(client_sock, sse_event, strlen(sse_event), 0);
        if (ret < 0) {
          // printf("Error sending SSE event: errno %d\n", errno);
          break;
        }
      } else {
        break;
      }
    }
  } else {
    // Not found response for other paths
    const char *not_found = "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 9\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Not Found";

    send(client_sock, not_found, strlen(not_found), 0);
  }

  // Close connection
  close(client_sock);
}

void sse_server(void *pvParameters) {
  PARAMETER_t *task_parameter = pvParameters;
  PARAMETER_t param;
  memcpy((char *)&param, task_parameter, sizeof(PARAMETER_t));
  printf("Start:param.port=%d\n", param.port);

  int addr_family = AF_INET;
  int ip_protocol = IPPROTO_IP;

  // Create a server socket instead of a client one
  struct sockaddr_in server_addr;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all interfaces
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(param.port);

  int server_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
  if (server_sock < 0) {
    printf("Unable to create socket: errno %d\n", errno);
    vTaskDelete(NULL);
  }

  // Set socket option to reuse address
  int opt = 1;
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Bind socket to address
  int err =
      bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err != 0) {
    printf("Socket unable to bind: errno %d\n", errno);
    close(server_sock);
    vTaskDelete(NULL);
  }

  // Start listening
  err = listen(server_sock, 5);
  if (err != 0) {
    printf("Error listening on socket: errno %d\n", errno);
    close(server_sock);
    vTaskDelete(NULL);
  }

  printf("SSE Server listening on port %d\n", param.port);

  // Send ready to receive notify
  xTaskNotifyGive(param.taskHandle);

  // Main server loop
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_sock < 0) {
      // printf("Unable to accept connection: errno %d\n", errno);
      continue;
    }

    serve_client(client_sock);
  }

  if (server_sock != -1) {
    shutdown(server_sock, 0);
    close(server_sock);
  }
  vTaskDelete(NULL);
}
