# esp32s2-mini
I got this board.   
![esp32-s2-mini-1](https://github.com/nopnop2002/esp-idf-net-logging/assets/6020549/80cf2630-c50b-46fb-9af4-d5313096def3)
![esp32-s2-mini-2](https://github.com/nopnop2002/esp-idf-net-logging/assets/6020549/f67769eb-8a28-4885-a4af-dec0e009b182)

A Type-C USB connector is mounted on the board, but a USB-TTL conversion chip is not mounted.   
Schematic was posted [here](https://www.wemos.cc/en/latest/_static/files/sch_s2_mini_v1.0.0.pdf).   
USB ports D- and D+ are connected to GPIO19/20.   
Since it uses the chip's USB-OTG function, it will not be recognized as a USB device even if you simply connect it to the host with a USB cable.   
To write firmware, press ButtonRST while pressing Button0, then release ButtonRST and then Button0.   
Now the device will enter DFU mode, the USB device (/dev/ttyACM0) will appear, and you can upload the firmware.   
If you press ButtonRST without pressing Button0, the USB device will disappear.   
In other words, USB can be used for writing, but not for STDOUT.   
In ESP32-S2, GPIO43/44 are U0TXD/U0RXD, but this GPIO does not appear on the pin.   
In other words, no matter what I do, it is a board that cannot be used for logging using a serial monitor.   
It is completely unusable for development or debugging.   
To see the logging output, this tool requires redirection to the network.   


# Installation
```Shell
git clone https://github.com/nopnop2002/esp-idf-net-logging
cd esp-idf-net-logging/esp32s2-nimi
idf.py menuconfig
idf.py flash
```


# Features of this board   
I used this tool to redirect log output to the network and view SoC information.   
This board has 4MB of FLASH and 2MB of PSRAM installed.   

![esp32s2-mini](https://github.com/nopnop2002/esp-idf-net-logging/assets/6020549/f36327cf-7e7b-4f1f-aa20-1eb16a0650ea)
