#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import select, socket
import argparse

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument('--port', type=int, help='udp port', default=6789)
	args = parser.parse_args()
	print("args.port={}".format(args.port))

	server_ip = "0.0.0.0" # Both Limited Broadcast/Directed Broadcast/Unicast
	#server_ip = "255.255.255.255" # Only Limited broadcast

	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	sock.bind( (server_ip, args.port) )
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


