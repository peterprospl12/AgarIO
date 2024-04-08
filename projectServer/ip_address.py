import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(("8.8.8.8", 80))
print("IP Address to connect: ", s.getsockname()[0])
s.close()