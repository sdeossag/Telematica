import socket
import time

HOST = "127.0.0.1"
PORT = 8080

def main():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))
            print("Conectado al servidor.")

            # Recibir mensaje de bienvenida
            msg = s.recv(1024).decode()
            print("Servidor:", msg.strip())

            # Login
            s.sendall(b"LOGIN admin 1234\n")
            print("Cliente: LOGIN admin 1234")
            print("Servidor:", s.recv(1024).decode().strip())

            # Solicitar temperatura
            s.sendall(b"GET TEMP\n")
            print("Cliente: GET TEMP")
            print("Servidor:", s.recv(1024).decode().strip())

            # Esperar un poco y cerrar
            time.sleep(1)
            s.sendall(b"QUIT\n")
            print("Cliente: QUIT")
            print("Servidor:", s.recv(1024).decode().strip())

    except Exception as e:
        print("Error:", e)

if __name__ == "__main__":
    main()
