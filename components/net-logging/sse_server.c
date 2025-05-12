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
#include "freertos/event_groups.h"

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
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

#define STOP_SERVING_CLIENT	( 1 << 0 )
#define STOPPED_SERVING_CLIENT	( 1 << 1 )

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
extern RingbufHandle_t xRingBufferSSE;
#else
extern MessageBufferHandle_t xMessageBufferSSE;
#endif

void serve_client(void *pvParameters) {
  void** params = (void**)pvParameters;
  const int* client_sock_ptr = (int*)params[0];
  int client_sock = *client_sock_ptr;
  const EventGroupHandle_t* client_serving_task_events_ptr = (EventGroupHandle_t*)params[1];
  EventGroupHandle_t client_serving_task_events = *client_serving_task_events_ptr;
  const size_t sse_html_size = sse_html_end - sse_html_start;

  // Receive HTTP request
  char request[1024];
  int req_len = recv(client_sock, request, sizeof(request) - 1, 0);
  //printf("request received: %.*s\n", req_len, request);
  if (req_len <= 0) {
    // printf("Connection closed or error\n");
    shutdown(client_sock, 0);
    close(client_sock);
    vTaskDelete(NULL);
  }

  // Null-terminate received data
  request[req_len] = 0;

  // Check if the request is for the root path (/)
  if (strstr(request, "GET / HTTP") != NULL) {
    // Prepare HTTP response
    char headers[256];
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n",
             sse_html_size);

    // Send header
    send(client_sock, headers, strlen(headers), 0);

    // Send file content
    send(client_sock, sse_html_start, sse_html_size, 0);
  }
  // Check if the request is for the SSE endpoint
  else if (strstr(request, "GET /log-events HTTP") != NULL) {
    //printf("SSE client connected\n");
    // Send SSE headers
    const char *headers = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/event-stream\r\n"
                              "retry: 1000\r\n"
                              "Cache-Control: no-cache\r\n"
                              "Connection: keep-alive\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "\r\n";

    send(client_sock, headers, strlen(headers), 0);
    //printf("serving SSE\n");

    // Keep connection open and send SSE events
    while ((xEventGroupGetBits(client_serving_task_events) & STOP_SERVING_CLIENT) == 0) {
      #if CONFIG_NET_LOGGING_USE_RINGBUFFER
      size_t received;
      char *buffer = (char *)xRingbufferReceive(xRingBufferSSE, &received, pdMS_TO_TICKS(10));
      #else
      char buffer[xItemSize];
      size_t received = xMessageBufferReceive(xMessageBufferSSE, buffer, sizeof(buffer), pdMS_TO_TICKS(10));
      #endif

      if (received > 0) {
        // Format the buffer content as an SSE event
        char sse_event[512];
        snprintf(sse_event, sizeof(sse_event), "event: log-line\ndata: %.*s\n\n", (int)received, buffer);

        #if CONFIG_NET_LOGGING_USE_RINGBUFFER
        vRingbufferReturnItem(xRingBufferSSE, (void *)buffer);
        #endif

        // Send the event
        int ret = send(client_sock, sse_event, strlen(sse_event), 0);
        if (ret < 0) {
          // printf("Error sending SSE event: errno %d\n", errno);
          break;
        }
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
  //printf("closing connection\n");
  shutdown(client_sock, 0);
  close(client_sock);
  xEventGroupSetBits(client_serving_task_events, STOPPED_SERVING_CLIENT );
  vTaskDelete(NULL);
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
  static EventGroupHandle_t client_serving_task_events;
  StaticEventGroup_t client_serving_task_events_data;
  client_serving_task_events = xEventGroupCreateStatic( &client_serving_task_events_data );
  TaskHandle_t client_task = NULL;
  static int client_sock;
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
      //printf("Unable to accept connection: errno %d\n", errno);
      continue;
    }

    // Stop serving the previous client, if any
    if (client_task != NULL) {
      xEventGroupSetBits(client_serving_task_events, STOP_SERVING_CLIENT );
      xEventGroupWaitBits(
        client_serving_task_events,
        STOPPED_SERVING_CLIENT,
        pdTRUE,
        pdFALSE,
        portMAX_DELAY
      );
      client_task = NULL;
    }

    // serve new client
    xEventGroupClearBits(client_serving_task_events, STOP_SERVING_CLIENT | STOPPED_SERVING_CLIENT );
    void* client_pvParams[] = {
      (void*)&client_sock,
      (void*)&client_serving_task_events
    };
    xTaskCreate(
      serve_client,
      "LOGS_SSE_SERVE_CLIENT",
      1024*4,
      (void*) client_pvParams,
      2,
      &client_task
    );
  }

  if (server_sock != -1) {
    shutdown(server_sock, 0);
    close(server_sock);
  }
  vTaskDelete(NULL);
}
