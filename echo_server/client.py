import time
import socket

HOST = '10.8.58.51'
PORT = 6000
counter = 0

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    while True:
        counter += 1
        s.sendall(f'ping {counter}'.encode())
        data = s.recv(1024)

        print('Received:', data.decode("utf-8"))
        time.sleep(1)
