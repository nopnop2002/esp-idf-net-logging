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
- SSE   

We can use Linux's rsyslogd as the logger.   
I referred to [this](https://github.com/MalteJ/embedded-esp32-component-udp_logging).

# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   
ESP-IDF V5.1 is required when using ESP32-C6.   


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
ESP32 works as a UDP client.   
![Image](https://github.com/user-attachments/assets/9f8b40d7-58ea-4a23-a22b-94a9f3883140)

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
ESP32 works as a TCP client.   
![Image](https://github.com/user-attachments/assets/43774f6d-bfd3-4e6c-b367-d001284943de)

You can use the mDNS hostname of such a TCP server instead of the IP address.   
tcp-server.local   


## Configuration for MQTT Redirect
ESP32 works as a MQTT client.   
![Image](https://github.com/user-attachments/assets/101b8094-bb1e-4322-b793-51d930c53f48)


## Configuration for HTTP Redirect
ESP32 works as a HTTP client.   
You can use mDNS host name for your http server.
Image](https://github.com/user-attachments/assets/02214da9-0fd8-4ff4-8da9-1343006ca530)


## Configuration for SSE Redirect
ESP32 works as a SSE server.   
![Image](https://github.com/user-attachments/assets/226c05a2-2629-450b-9522-8655b9bb6ac6)


## Disable Logging to STDOUT
![Image](https://github.com/user-attachments/assets/5f982b89-18eb-483b-acbb-31718b3aa6a5)

## Use xRingBuffer as IPC
![Image](https://github.com/user-attachments/assets/2a602782-e590-4cea-99fb-ca88cf0aab5a)

Both xMessageBuffer and xRingBuffer are interprocess communication (IPC) components provided by ESP-IDF.   
Several drivers provided by ESP-IDF use xRingBuffer.   
This project uses xMessageBuffer by default.   
If you use this project at the same time as a driver that uses xRingBuffer, using xRingBuffer uses less memory.   
Memory usage status can be checked with ```idf.py size-files```.   

# View logging   
You can view the logging using python code or various tools.   
- for UDP   
	![net-logging-udp](https://user-images.githubusercontent.com/6020549/182273454-834cedb7-d884-4a89-823f-13e5d7a1c6b5.jpg)   
	You can use ```netcat``` as server.   
	![netcat-udp](https://user-images.githubusercontent.com/6020549/198207929-649537ae-0c4e-45ed-8c88-7167505b124e.jpg)   
	We can use [this](https://apps.microsoft.com/detail/9nblggh52bt0) as Logging Viewer.   
	Note that the most recent logging is displayed at the __top__.   
	![windows-udp-server](https://github.com/user-attachments/assets/0313b845-1a8d-4e06-9a02-1bb91de895d2)   
	We can also use [this](https://apps.microsoft.com/detail/9p4nn1x0mmzr) as Logging Viewer.   
	Note that the most recent logging is displayed at the __buttom__.   
	I like this one better.   
	![windows-udp-server-11](https://github.com/user-attachments/assets/1d373809-7774-4e84-9256-2f81ec74368d)   
	There are others if you look for them.   

- for TCP   
	![net-logging-tcp](https://user-images.githubusercontent.com/6020549/182273510-92cf406b-7197-4cfe-9ff6-5421dc8eea8d.jpg)   
	You can use ```netcat``` as server.   
	![netcat-tcp](https://user-images.githubusercontent.com/6020549/198230565-4fece92e-349f-4555-aba6-2196d3b6c040.jpg)   
	We can use [this](https://sourceforge.net/projects/sockettest/) as Logging Viewer.   
	![windows-tcp-server-1](https://github.com/user-attachments/assets/76baaaba-2453-47d1-8acc-6f045413fdcc)   

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

- for SSE   
	Open a browser and enter the IP address of the ESP32 in the address bar.
	![Image](https://github.com/user-attachments/assets/15a45454-03c1-49be-a5fa-1e1328c24d89)


# Using linux rsyslogd as logger   
We can forward logging to rsyslogd on Linux machine.   
Configure with protocol = UDP and port number = 514.   
![Image](https://github.com/user-attachments/assets/7d7c6cc2-2f58-40ec-8a3d-afbc80305403)

The rsyslog server on linux can receive logs from outside.   
Execute the following command on the Linux machine that will receive the logging data.   
I used Ubuntu 22.04.   

```
$ cd /etc/rsyslog.d/

$ sudo vi 99-remote.conf
module(load="imudp")
input(type="imudp" port="514")

if $fromhost-ip != '127.0.0.1' and $fromhost-ip != 'localhost' then {
    action(type="omfile" file="/var/log/remote")
    stop
}

$ sudo ufw enable
Firewall is active and enabled on system startup

$ sudo ufw allow 514/udp
Rule added
Rule added (v6)

$ sudo ufw allow 22/tcp
Rule added
Rule added (v6)

$ sudo systemctl restart rsyslog

$ ss -nulp | grep 514
UNCONN 0      0            0.0.0.0:514        0.0.0.0:*
UNCONN 0      0               [::]:514           [::]:*

$ sudo ufw status
Status: active

To                         Action      From
--                         ------      ----
514/udp                    ALLOW       Anywhere
22/tcp                     ALLOW       Anywhere
514/udp (v6)               ALLOW       Anywhere (v6)
22/tcp (v6)                ALLOW       Anywhere (v6)
```

Logging from esp-idf goes to /var/log/remote.   
```
$ tail -f /var/log/remote
May  8 14:06:09 I (6688) MAIN: This is info level
May  8 14:06:09 W (6698) MAIN: This is warning level
May  8 14:06:09 E (6698) MAIN: This is error level
May  8 14:06:09 I (6698) MAIN: freeRTOS version:V10.5.1
May  8 14:06:09 I (6708) MAIN: NEWLIB version:4.3.0
May  8 14:06:09 I (6708) MAIN: lwIP version:2-2-0-0
May  8 14:06:09 I (6708) MAIN: ESP-IDF version:v5.4.1-dirty
May  8 14:06:09 I (6718) MAIN: chip model is 1,
May  8 14:06:09 I (6718) MAIN: chip with 2 CPU cores, WiFi/BT/BLE
May  8 14:06:09 I (6718) MAIN: silicon revision 100
May  8 14:06:09 I (6728) MAIN: 2MB external flash
May  8 14:06:09 I (6728) main_task: Returned from app_main()
May  8 14:06:09 I (7398) wifi:
May  8 14:06:09 192.168.10.130  <ba-add>idx:0 (ifx:0, f8:b7:97:36:de:52), tid:5, ssn:0, winSize:64
May  8 14:06:09 192.168.10.130
May  8 14:06:10 I (7498) wifi:
May  8 14:06:10 192.168.10.130  <ba-add>idx:1 (ifx:0, f8:b7:97:36:de:52), tid:0, ssn:4, winSize:64
May  8 14:06:10 192.168.10.130
```

# Disable ANSI Color control
You can disable this if you are unable to display ANSI color codes correctly.   
![ANSI-Color](https://github.com/user-attachments/assets/c36b5f74-e85a-48c0-b498-5cb5301f0d24)   

Enable ANSI Color control   
![windows-udp-server](https://github.com/user-attachments/assets/0313b845-1a8d-4e06-9a02-1bb91de895d2)   

Disable ANSI Color control   
![windows-udp-server-2](https://github.com/user-attachments/assets/38d2c698-5690-402c-9419-a7dfe639e4d7)   


# API   
Use one of the following.   
Subsequent logging will be redirected.   
```
esp_err_t udp_logging_init(char *ipaddr, unsigned long port, int16_t enableStdout);
esp_err_t tcp_logging_init(char *ipaddr, unsigned long port, int16_t enableStdout);
esp_err_t mqtt_logging_init(char *url, char *topic, int16_t enableStdout);
esp_err_t http_logging_init(char *url, int16_t enableStdout);
esp_err_t sse_logging_init(unsigned long port, int16_t enableStdout);
```

It is possible to use multiple protocols simultaneously.   
The following example uses UDP and SSE together.   
```
udp_logging_init("255.255.255.255", 6789, true);
sse_logging_init(8080, true);
```


# How to use this component in your project   
Create idf_component.yml in the same directory as main.c.   
```
YourProject --+-- CMakeLists.txt
              +-- main --+-- main.c
                         +-- CMakeLists.txt
                         +-- idf_component.yml
```

Contents of idf_component.yml.
```
dependencies:
  nopnop2002/net_logging:
    path: components/net-logging/
    git: https://github.com/nopnop2002/esp-idf-net-logging.git
```

When you build a projects esp-idf will automaticly fetch repository to managed_components dir and link with your code.   
```
YourProject --+-- CMakeLists.txt
              +-- main --+-- main.c
              |          +-- CMakeLists.txt
              |          +-- idf_component.yml
              +-- managed_components ----- nopnop2002__net_logging
```

