#!/usr/bin/env python3

import signal
import socket
import select

server_ip = "0.0.0.0"
server_port = 8080
listen_num = 5
buffer_size = 1024

def handler(signal, frame):
	global isRunning
	#print('handler')
	isRunning = False

def main():
	signal.signal(signal.SIGINT, handler)

	print("+==========================+")
	print("| ESP32 TCP Logging Server |")
	print("+==========================+")
	print("")

	tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	tcp_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	tcp_server.bind((server_ip, server_port))
	tcp_server.listen(listen_num)
	client,address = tcp_server.accept()
	#print("Connected!! [ Source : {}]".format(address))
	client.setblocking(0)

	global isRunning
	isRunning = True
	while isRunning:
		ready = select.select([client], [], [], 1)
		#print("ready={}".format(ready[0]))
		if ready[0]:
			data = client.recv(buffer_size)
			if (type(data) is bytes):
				data = data.decode('utf-8')
				#print("[*] Received Data : {}".format(data))
				print(data, end='')
	
	client.close()

if __name__ == "__main__":
	main()
