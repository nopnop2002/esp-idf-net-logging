#!/usr/bin/env python3

import sys
import select, socket

UDP_IP = "0.0.0.0" # Both Limited Broadcast/Directed Broadcast/Unicast
#UDP_IP = "255.255.255.255" # Only Limited broadcast
UDP_PORT = 6789

def main():
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	sock.bind( (UDP_IP, UDP_PORT) )
	sock.setblocking(0)

	print("+==========================+")
	print("| ESP32 UDP Logging Server |")
	print("+==========================+")
	print("")

	while True:
		result = select.select([sock],[],[])
		data = result[0][0].recv(1024)
		if (type(data) is bytes):
			data = data.decode('utf-8')
		print(data, end='')

if __name__ == "__main__":
	main()

