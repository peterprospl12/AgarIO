#!/bin/bash
curl -o client.py https://raw.githubusercontent.com/peterprospl12/AgarIO/Master/projectServer/ip_address.py
curl -o client.py https://raw.githubusercontent.com/peterprospl12/AgarIO/Master/projectServer/server.c

python3 ip_address.py
gcc server.c -lm -lpthread -o server
./server