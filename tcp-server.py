#!/usr/bin/env python3

import socket

server_ip = "0.0.0.0"
server_port = 8080
listen_num = 5
buffer_size = 1024

def main():
	print("+==========================+")
	print("| ESP32 TCP Logging Server |")
	print("+==========================+")
	print("")

	tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	tcp_server.bind((server_ip, server_port))
	tcp_server.listen(listen_num)
	client,address = tcp_server.accept()
	print("[*] Connected!! [ Source : {}]".format(address))

	while True:
		#client,address = tcp_server.accept()
		#print("[*] Connected!! [ Source : {}]".format(address))
	
		data = client.recv(buffer_size)
		if (type(data) is bytes):
			data = data.decode('utf-8')
			#print("[*] Received Data : {}".format(data))
			print(data, end='')
	
		client.send(b"ACK!!")

	client.close()

if __name__ == "__main__":
	main()
