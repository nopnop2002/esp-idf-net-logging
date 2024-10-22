#!/usr/bin/env python3

import signal
import socket
import select
import argparse

def handler(signal, frame):
	global running
	#print('handler')
	running = False

if __name__ == "__main__":
	signal.signal(signal.SIGINT, handler)
	running = True

	parser = argparse.ArgumentParser()
	parser.add_argument('--port', type=int, help='tcp port', default=8080)
	args = parser.parse_args()
	print("args.port={}".format(args.port))

	print("+==========================+")
	print("| ESP32 TCP Logging Server |")
	print("+==========================+")
	print("")

	server_ip = "0.0.0.0"
	listen_num = 5
	buffer_size = 1024

	tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	tcp_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	tcp_server.bind((server_ip, args.port))
	tcp_server.listen(listen_num)
	client,address = tcp_server.accept()
	#print("Connected!! [ Source : {}]".format(address))
	client.setblocking(0)

	while running:
		ready = select.select([client], [], [], 1)
		#print("ready={}".format(ready[0]))
		if ready[0]:
			data = client.recv(buffer_size)
			if (type(data) is bytes):
				data = data.decode('utf-8')
				#print("[*] Received Data : {}".format(data))
				print(data, end='')
	
	client.close()
