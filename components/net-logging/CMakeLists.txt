idf_component_register(
  SRCS
    "net_logging.c"
    "udp_client.c"
    "tcp_client.c"
    "mqtt_pub.c"
    "http_client.c"
    "sse_server.c"
  INCLUDE_DIRS "."
  REQUIRES
    "esp_http_client"
    "esp_ringbuf"
    "mqtt"
  EMBED_TXTFILES
    "assets/sse.html"
  )
