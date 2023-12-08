# esp-idf-net-logging
Redirect esp-idf logging to the network.


esp-idf has a Logging library.   
The Logging library contains the ```esp_log_set_vprintf``` function.   
By default, log output goes to UART0.    
This function can be used to redirect log output to some other destination, such as file or network.    

I made a project that uses 3 UARTs of ESP32.   
But I was in trouble because there was no logging output destination.   
So I made this.   


The following protocols are available for this project.
- UDP   
- TCP   
- MQTT   
- HTTP(POST)   

I referred to [this](https://github.com/MalteJ/embedded-esp32-component-udp_logging).

# Software requirements
esp-idf v4.4/v5.0.   


# Installation
```Shell
git clone https://github.com/nopnop2002/esp-idf-net-logging
cd esp-idf-net-logging/basic
idf.py menuconfig
idf.py flash
```

# Configuration   
![config-top](https://user-images.githubusercontent.com/6020549/151915919-d6f19861-8d48-4630-aeed-aab819929dc6.jpg)

## Configuration for UDP Redirect
![config-udp](https://github.com/nopnop2002/esp-idf-net-logging/assets/6020549/5a9914ff-53a7-44f9-9ebf-08641c5123da)

There are the following four methods for specifying the UDP Address.
- Limited broadcast address   
 The address represented by 255.255.255.255, or \<broadcast\>, cannot cross the router.   
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
![config-tcp](https://github.com/nopnop2002/esp-idf-net-logging/assets/6020549/1f3a2609-2cce-498b-96fd-e5cf598552af)

You can use mDNS host name for your tcp server.

## Configuration for MQTT Redirect
![config-mqtt](https://github.com/nopnop2002/esp-idf-net-logging/assets/6020549/d27be5d2-6a1a-4c5f-86c9-6cdf4394d137)


## Configuration for HTTP Redirect
![config-http](https://github.com/nopnop2002/esp-idf-net-logging/assets/6020549/ea09b7e6-a95a-4351-8fb8-d6d9a9c398cb)

You can use mDNS host name for your http server.

## Disable Logging to STDOUT
![config-stdout](https://github.com/nopnop2002/esp-idf-net-logging/assets/6020549/c8516a79-4c55-414f-b0b6-41eff0006e72)

## Use xRingBuffer as IPC
![config-xRingBuffer](https://github.com/nopnop2002/esp-idf-net-logging/assets/6020549/53aef0cc-0e44-4f19-a10c-d55bc78ef091)

Both xMessageBuffer and xRingBuffer are interprocess communication components provided by ESP-IDF.   
Several drivers provided by ESP-IDF use xRingBuffer.   
This project uses xMessageBuffer by default.   
If you use this project at the same time as a driver that uses xRingBuffer, using xRingBuffer uses less memory.   
Memory usage status can be checked with ```idf.py size-files```.   

# View logging   
You can see the logging using python code or mosqutto client.   
- for UDP   
![net-logging-udp](https://user-images.githubusercontent.com/6020549/182273454-834cedb7-d884-4a89-823f-13e5d7a1c6b5.jpg)   
You can use ```netcat``` as server.   
![netcat-udp](https://user-images.githubusercontent.com/6020549/198207929-649537ae-0c4e-45ed-8c88-7167505b124e.jpg)

- for TCP   
![net-logging-tcp](https://user-images.githubusercontent.com/6020549/182273510-92cf406b-7197-4cfe-9ff6-5421dc8eea8d.jpg)   
You can use ```netcat``` as server.   
![netcat-tcp](https://user-images.githubusercontent.com/6020549/198230565-4fece92e-349f-4555-aba6-2196d3b6c040.jpg)

- for MQTT   
 The wifi logging is output in two parts.   
 First time:W (7060) wifi:   
 Second time:Characters after that   
 In MQTT and HTTP, it is displayed separately in two.   
__If you use broker.emqx.io, continuous Logging will drop.__   
![net-logging-mqtt](https://user-images.githubusercontent.com/6020549/182273560-fc1931bf-71f7-4751-a57d-680312a93391.jpg)   
__Using a local MQTT server is stable.__   
You can use [this](https://github.com/nopnop2002/esp-idf-mqtt-broker) as a broker.   
![net-logging-mqtt-local](https://user-images.githubusercontent.com/6020549/182275982-63581071-c0b0-4851-a928-b5e2286b6893.jpg)

- for HTTP   
![net-logging-http](https://user-images.githubusercontent.com/6020549/182273590-26281a3c-c048-466a-9d00-764981f89b49.jpg)


# API   
Use one of the following.   
Subsequent logging will be redirected.   
```
esp_err_t udp_logging_init(char *ipaddr, unsigned long port, int16_t enableStdout);
esp_err_t tcp_logging_init(char *ipaddr, unsigned long port, int16_t enableStdout);
esp_err_t mqtt_logging_init(char *url, char *topic, int16_t enableStdout);
esp_err_t http_logging_init(char *url, int16_t enableStdout);
```
