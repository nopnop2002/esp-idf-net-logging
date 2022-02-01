# esp-idf-net-logging
Redirect esp-idf logging to the network.


esp-idf has a Logging library.   
The Logging library contains the "esp_log_set_vprintf" function.   
By default, log output goes to UART0.    
This function can be used to redirect log output to some other destination, such as file or network.    

I made a project that uses 3 QUARTs of ESP32.   
But I was in trouble because there was no logging output destination.   
So I made this.   


The following protocols are available for this project.
- UDP   
- TCP   
- MQTT   
- HTTP(POST)   

I referred to [this](https://github.com/MalteJ/embedded-esp32-component-udp_logging).


# Installation
```Shell
git clone https://github.com/nopnop2002/esp-idf-net-logging
cd esp-idf-net-logging
idf.py set-target {esp32/esp32s2/esp32c3}
idf.py menuconfig
idf.py flash
```

# Configuration   
![config-top](https://user-images.githubusercontent.com/6020549/151915919-d6f19861-8d48-4630-aeed-aab819929dc6.jpg)

## Configuration for UDP Redirect
![config-udp](https://user-images.githubusercontent.com/6020549/151915950-87d97cee-1082-4a37-96c5-77958bee4051.jpg)

There are the following four methods for specifying the UDP Address.
- Limited broadcast address   
 The address represented by 255.255.255.255, or <broadcast>, cannot cross the router.   
 Both the sender and receiver must specify a Limited broadcast address.   

- Directed broadcast address   
 It is possible to cross the router with an address that represents only the last octet as 255, such as 192.168.10.255.   
 Both the sender and receiver must specify the Directed broadcast address.   
 __Note that it is possible to pass through the router.__   

- Multicast address   
 Data is sent to all PCs belonging to a specific group using a special address (224.0.0.0 to 239.255.255.255) called a multicast address.   
 I've never used it, so I don't know anything more.

- Unicast address   
 It is possible to cross the router with an address that specifies all octets, such as 192.168.10.41.
 Both the sender and receiver must specify the Unicast address.

## Configuration for TCP Redirect
![config-tcp](https://user-images.githubusercontent.com/6020549/151915971-191f4e66-d1b4-41c0-a2d9-1822c7383bb9.jpg)


## Configuration for MQTT Redirect
![config-mqtt](https://user-images.githubusercontent.com/6020549/151916002-84523ad7-e591-4a55-aad6-997dbda3bf71.jpg)


## Configuration for HTTP Redirect
![config-http](https://user-images.githubusercontent.com/6020549/151916036-7af01f41-7161-4e09-9b19-beb5676b0e90.jpg)


# View logging   
You can see the logging using python code or mosqutto client.   
- for UDP   
![net-logging-udp](https://user-images.githubusercontent.com/6020549/151916070-184e5ad7-dc70-4536-bde0-b20e99439f09.jpg)
- for TCP   
![net-logging-tcp](https://user-images.githubusercontent.com/6020549/151916076-1ce6167f-a46a-42fd-a6e5-eab9bde6188b.jpg)
- for MQTT   
 The wifi logging is output in two parts.   
 First time:W (7060) wifi:   
 Second time:Characters after that   
 In MQTT and HTTP, it is displayed separately in two.   
![net-logging-mqtt](https://user-images.githubusercontent.com/6020549/151916086-72f83d39-cd85-41e8-a2ba-eee95573c2b6.jpg)
- for HTTP   
![net-logging-http](https://user-images.githubusercontent.com/6020549/151916096-8c11920e-b88b-473b-a86a-a7f04eb6c978.jpg)


# API   
Use one of the following.   
Subsequent logging will be redirected.   
```
esp_err_t udp_logging_init(char *ipaddr, unsigned long port, int16_t enableStdout);
esp_err_t tcp_logging_init(char *ipaddr, unsigned long port, int16_t enableStdout);
esp_err_t mqtt_logging_init(char *url, char *topic, int16_t enableStdout);
esp_err_t http_logging_init(char *url, int16_t enableStdout);
```
