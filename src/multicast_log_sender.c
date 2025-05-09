/*
    Multicast Log Sender

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
#include "freertos/ringbuf.h"
#else
#include "freertos/message_buffer.h"
#endif

#include "esp_system.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h" // for inet_addr_from_ip4addr
#include "lwip/netdb.h" // for getaddrinfo
#include "esp_netif.h"
#include "net_logging_priv.h"

#define MULTICAST_TTL (1) // 1=don't leave the subnet
#define USE_DEFAULT_IF (1) // 1=bind to default interface, 0=bind to specific interface
#define RETRY_TIMEOUT_MS (3000) // retry timeout in ms

#define TAG "multicast_log_sender: "

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
extern RingbufHandle_t xRingBufferMulticast;
#else
extern MessageBufferHandle_t xMessageBufferMulticast;
#endif

static int create_multicast_ipv4_socket(struct in_addr bind_iaddr, uint16_t port)
{
    int sock = -1;
    int err = 0;
    char addrbuf[32] = {0};

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0)
    {
        printf(TAG  "Failed to create socket. Error %d", errno);
        return -1;
    }
    // enable SO_REUSEADDR so servers restarted on the same ip addresses
    // do not require waiting for 2 minutes while the socket is in TIME_WAIT
    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        printf(TAG  "setsockopt(SO_REUSEADDR) failed");
    }

    // Configure source interface (bind to interface)
    struct sockaddr_in saddr = {0};
    saddr.sin_addr.s_addr = bind_iaddr.s_addr; // what interface IP to bind to.  Can be htonl(INADDR_ANY)
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(port);

    inet_ntoa_r(saddr.sin_addr.s_addr, addrbuf, sizeof(addrbuf) - 1);
    ESP_LOGI(TAG, "Binding to interface %s...", addrbuf);
    err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (err < 0)
    {
        printf(TAG  "Failed to bind socket. Error %d", errno);
        goto err;
    }

    // Assign multicast TTL (set separately from normal interface TTL)
    uint8_t ttl = MULTICAST_TTL;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(uint8_t));
    if (err < 0)
    {
        printf(TAG  "Failed to set IP_MULTICAST_TTL. Error %d", errno);
        goto err;
    }

    // All set, socket is configured for sending
    return sock;

err:
    close(sock);
    return -1;
}

static int multicast_setup(int *sock_out, struct addrinfo **res_out, struct in_addr src_addr, const char* mcast_addr, uint16_t port)
{
    int err = 0;
    int sock = create_multicast_ipv4_socket(src_addr, port);
    if (sock < 0)
    {
        printf(TAG  "Failed to create socket. Error %d", errno);
        return -1;
    }

    struct addrinfo hints = {
        .ai_flags = AI_PASSIVE,
        .ai_socktype = SOCK_DGRAM,
    };
    hints.ai_family = AF_INET; // For an IPv4 socket
    struct addrinfo *res;

    err = getaddrinfo(mcast_addr, NULL, &hints, &res);
    if (0 != err) {
        ESP_LOGE(TAG, "Failed getaddrinfo");
        return -1;
    }

    ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(port);
    *res_out = res;
    *sock_out = sock;
    return err;
}

// UDP Multicast Log Sender Task
void multicast_log_sender(void *pvParameters) {
    PARAMETER_t *param = pvParameters;

    while (1) // Outer while loop to create socket
    {
        // Configure source interface
        struct in_addr src_addr = {0};
#if USE_DEFAULT_IF
        src_addr.s_addr = htonl(INADDR_ANY); // Bind the socket to any address
#else
        esp_netif_ip_info_t eth_ip_info;
        esp_netif_t *netif_eth = esp_netif_get_handle_from_ifkey("ETH_DEF");
        esp_netif_get_ip_info(netif_eth, &eth_ip_info);
        inet_addr_from_ip4addr(&src_addr, &eth_ip_info.ip);
#endif
        /* create the socket */
        int sock = -1;
        struct addrinfo *res_toFree = NULL;
        if (0 != multicast_setup(&sock, &res_toFree, src_addr, param->ipv4 ,param->port))
        {
            printf(TAG "Failed to setup");
            // wait and then try again
            vTaskDelay(RETRY_TIMEOUT_MS / portTICK_PERIOD_MS);
            continue;
        }

        while (1)  // Inner while loop to send data
        {
#if CONFIG_NET_LOGGING_USE_RINGBUFFER
            size_t received;
            char *buffer = (char *)xRingbufferReceive(xRingBufferMulticast, &received, portMAX_DELAY);
            //printf("xRingBufferReceive received=%d\n", received);
#else
            char buffer[xItemSize];
            size_t received = xMessageBufferReceive(xMessageBufferMulticast, buffer, sizeof(buffer), portMAX_DELAY);
            //printf("xMessageBufferReceive received=%d\n", received);
#endif
            if (received > 0) {
                //printf("xMessageBufferReceive buffer=[%.*s]\n",received, buffer);
                int sendto_ret = sendto(sock, buffer, received, 0, res_toFree->ai_addr, res_toFree->ai_addrlen);
                if (sendto_ret < 0)
                {
                    printf(TAG "sendto failed. errno: %d\n", errno);
                    // wait and then try again
                    vTaskDelay(RETRY_TIMEOUT_MS / portTICK_PERIOD_MS);
                    break; // break out of the inner while loop, back to the outer while loop to try to create the socket again
                }

#if CONFIG_NET_LOGGING_USE_RINGBUFFER
                vRingbufferReturnItem(xRingBufferMulticast, (void *)buffer);
#endif
            }
            else {
                printf(TAG "BufferReceive fail\n");
                // wait and then try again
                vTaskDelay(RETRY_TIMEOUT_MS / portTICK_PERIOD_MS);
                break; // break out of the inner while loop, back to the outer while loop to try to create the socket again
            }
        } // end inner while

        printf(TAG "close socket and restart...\n");
        freeaddrinfo(res_toFree); // free the addrinfo struct
        shutdown(sock, 0);
        // Close socket
        close(sock);
    } // end outer while

    // Cleanup (should never reach here)
    if (param != NULL) {
        free(param);
        param = NULL;
    }
    vTaskDelete(NULL);
}

