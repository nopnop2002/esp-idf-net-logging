# UART Select Example

[Here](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/uart/uart_select) is the official example.    

In the official example, log output goes to UART0, so you can't use UART0 to communicate with peripherals.   

Redirecting esp-idf logging to the network allows UART0 to communicate with peripherals.   

# Installation
```Shell
git clone https://github.com/nopnop2002/esp-idf-net-logging
cd esp-idf-net-logging/uart_select
idf.py menuconfig
idf.py flash
```

# How to use   
- Connect the UART-TTL converter to the ESP32.   
	|ESP32||Converter|
	|:-:|:-:|:-:|
	|GPIO4|--|RXD|
	|GPIO5|--|TXD|
	|GND|--|GND|

- Connect the UART-TTL converter to the HOST.   
	I used windows10 as HOST.   
	You can use Linux as HOST.   

- Configuration for Redirect using menuconfig.   

- Flash firmware.   

- Start logging server like udp-server.py.   

- Start terminal application like TeraTerm.   
	Open the serial port at 115200bps.   

- Enter any key in the terminal application.   

- Logging is redirected.   
	![uart_select](https://user-images.githubusercontent.com/6020549/197926926-5198c6c6-4a48-43ce-8a1f-3bf2a8caa062.jpg)

