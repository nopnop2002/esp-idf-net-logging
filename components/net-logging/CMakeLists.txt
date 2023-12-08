set(component_srcs "net_logging.c" "udp_client.c" "tcp_client.c" "mqtt_pub.c" "http_client.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "."
                       REQUIRES esp_http_client
                       REQUIRES esp_ringbuf
                       REQUIRES mqtt)
