import socket
import threading
import tkinter as tk
from tkinter import messagebox

# ---------------- CONFIGURACIÓN ----------------
HOST = "100.24.14.26"
PORT = 8080
is_admin = False  # Cambia a False si quieres probar un cliente normal

# ---------------- CLASE CLIENTE ----------------
class RobotClient:
    def __init__(self, master):
        self.master = master
        master.title("RTLP Robot Client")
        master.geometry("300x300")
        master.resizable(False, False)

        # Estado inicial del robot
        self.x, self.y = 5, 5
        self.variables = {"TEMP": 0, "HUM": 0, "PRES": 0, "CO2": 0}

        # Interfaz
        tk.Label(master, text="Robotsito", font=("Arial", 12, "bold")).pack(pady=5)
        self.canvas = tk.Canvas(master, width=200, height=200, bg="white")
        self.canvas.pack()
        self.robot_icon = self.canvas.create_oval(90, 90, 110, 110, fill="red")

        # Variables atmosféricas
        self.data_label = tk.Label(master, text="", font=("Arial", 10))
        self.data_label.pack(pady=5)

        # Botones de movimiento (solo admin)
        if is_admin:
            frame = tk.Frame(master)
            frame.pack(pady=5)
            master.geometry("400x400")

            up_btn = tk.Button(frame, text="↑", width=5, command=lambda: self.move("UP"))
            left_btn = tk.Button(frame, text="←", width=5, command=lambda: self.move("LEFT"))
            right_btn = tk.Button(frame, text="→", width=5, command=lambda: self.move("RIGHT"))
            down_btn = tk.Button(frame, text="↓", width=5, command=lambda: self.move("DOWN"))

            up_btn.grid(row=0, column=1, pady=2)
            left_btn.grid(row=1, column=0, padx=2)
            right_btn.grid(row=1, column=2, padx=2)
            down_btn.grid(row=2, column=1, pady=2)

            for i in range(3):
                frame.grid_columnconfigure(i, pad=5)
                frame.grid_rowconfigure(i, pad=5)


        # Conexión
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            self.socket.connect((HOST, PORT))
        except:
            messagebox.showerror("Error", "No se pudo conectar con el servidor.")
            master.destroy()
            return

        threading.Thread(target=self.receive_data, daemon=True).start()
        self.login()

    # ---------------- MÉTODOS ----------------
    def login(self):
        msg = self.socket.recv(1024).decode().strip()
        print("Servidor:", msg)
        self.socket.sendall(b"LOGIN admin 1234\n")

    def move(self, direction):
        try:
            self.socket.sendall(f"MOVE {direction}\n".encode())
            # Simula movimiento visual inmediato
            dx, dy = 0, 0
            if direction == "UP": dy = -10
            elif direction == "DOWN": dy = 10
            elif direction == "LEFT": dx = -10
            elif direction == "RIGHT": dx = 10
            self.canvas.move(self.robot_icon, dx, dy)
        except:
            messagebox.showerror("Error", "Conexión perdida.")
            self.master.destroy()


    def receive_data(self):
        while True:
            try:
                data = self.socket.recv(1024).decode().strip()
                if not data:
                    break
                print("Servidor:", data)
                if data.startswith("DATA"):
                    self.update_variables(data)
            except:
                break

    def update_variables(self, data):
        try:
            parts = data.split()
            for part in parts[1:]:
                key, value = part.split("=")
                self.variables[key] = value
            text = f"TEMP: {self.variables['TEMP']}°C\nHUM: {self.variables['HUM']}%\nPRES: {self.variables['PRES']}hPa\nCO2: {self.variables['CO2']}ppm"
            self.data_label.config(text=text)
            self.canvas.move(self.robot_icon, 5, 0)  # Simula movimiento leve
        except Exception as e:
            print("Error al actualizar:", e)


# ---------------- MAIN ----------------
if __name__ == "__main__":
    root = tk.Tk()
    app = RobotClient(root)
    root.mainloop()
