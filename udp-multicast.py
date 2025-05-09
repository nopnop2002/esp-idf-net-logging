#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Multicast client
# Adapted from: http://chaos.weblogs.us/archives/164

import socket
import platform
import struct
import argparse

ANY = "0.0.0.0" 

DEF_ADDR = "239.2.1.2"
DEF_PORT = 2054
DEF_IFACE = ANY

def run_multicast(port, addr, iface):

	# Create a UDP socket
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

	# Allow multiple sockets to use the same PORT number
	try:
		sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	except AttributeError:
		pass

	sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 32) 
	sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)

	# Bind to the port that we know will receive multicast data
 	# linux binds to multicast address, windows to interface address
	ip_bind = iface if (platform.system() == "Windows") else addr
	sock.bind((ip_bind, port))
 
 	# IP_MULTICAST_IF: force sending network traffic over specific network adapter
	if (iface != ANY):
		sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(iface))

	mreq = struct.pack(
		'4sl' if (iface == ANY) else '4s4s',
		socket.inet_aton(addr),
		socket.INADDR_ANY if (iface == ANY) else socket.inet_aton(iface))
 
	# Tell the kernel that we want to add ourselves to a multicast group
	# The address for the multicast group is the third param
	sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
 
	# setblocking(False) is equiv to settimeout(0.0) which means we poll the socket.
	# But this will raise an error if recv() or send() can't immediately find or send data. 
	sock.setblocking(False)

	print("+================================+")
	print("| ESP32 Multicast Logging Server |")
	print("+================================+")
	print("")

	while 1:
		try:
			data, addr = sock.recvfrom(1024)
		except socket.error as e:
			pass
		else:
			print("From:", addr, end=' ')
			if (type(data) is bytes):
				data = data.decode('utf-8')
			if not data.endswith('\n'):
				data += '\n'
			print(data, end='')


if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument('--port', type=int, help='udp multicast port', default=DEF_PORT)
	parser.add_argument('--addr', type=str, help='udp multicast address', default=DEF_ADDR)
	parser.add_argument('--iface', type=str, help='host interface to bind to (default bind to all)', default=DEF_IFACE)
	args = parser.parse_args()
	print("args.port={}".format(args.port))
	print("args.iface={}".format(args.iface))
	print("args.addr={}".format(args.addr))
	run_multicast(args.port, args.addr, args.iface)