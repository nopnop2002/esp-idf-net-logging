set(component_srcs 
    "src/net_logging.c" 
    "src/udp_client.c" 
    "src/tcp_client.c" 
    "src/mqtt_pub.c" 
    "src/http_client.c"
    "src/sse_server.c"
)

idf_component_register(
    SRCS "${component_srcs}"
    INCLUDE_DIRS 
        "include"
    PRIV_REQUIRES 
        esp_http_client
        esp_ringbuf
        mqtt
        esp_hw_support
    EMBED_TXTFILES
        "src/assets/sse.html"
)
