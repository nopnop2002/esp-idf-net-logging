set(component_srcs 
    "src/net_logging.c" 
    "src/udp_client.c" 
    "src/tcp_client.c" 
    "src/mqtt_pub.c" 
    "src/http_client.c"
)

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_http_client
                       REQUIRES esp_ringbuf
                       REQUIRES mqtt
                       REQUIRES esp_hw_support)
