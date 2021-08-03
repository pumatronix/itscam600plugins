import socket

# This server echoes back anything it receives in the port PORT.

# It only accepts one connection at time and will, while the
# client keeps the connection openned, echo back anything
# it receives. Once the connection is closed it will stay
# oppened for future connections.

# To access stdout/stderr run 'docker logs <CONTAINER_ID> --tail 10 -f'

HOST = '0.0.0.0'
PORT = 6000

def perform_echo(connection):
    while True:
        data = connection.recv(1024)

        if not data: break

        print('Received/echoed:', data.decode("UTF-8"))
        connection.sendall(data)

def connect(socket):
    while True:
        conn, addr = socket.accept()
        print('Connected by', addr)

        with conn: perform_echo(conn)

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        print('Bind port:', PORT)
        s.bind((HOST, PORT))
        s.listen()
        connect(s)

main()
