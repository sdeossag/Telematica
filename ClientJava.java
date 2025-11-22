import java.io.*;
import java.net.*;

public class ClientJava {
    public static void main(String[] args) {
        String host = "100.24.14.26";
        int port = 8080;

        try (Socket socket = new Socket(host, port);
             BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
             PrintWriter out = new PrintWriter(socket.getOutputStream(), true)) {

            System.out.println("Conectado al servidor");

            // Recibir mensaje de bienvenida
            System.out.println("Servidor: " + in.readLine());

            // Login
            out.println("LOGIN admin 1234");
            System.out.println("Cliente: LOGIN admin 1234");
            System.out.println("Servidor: " + in.readLine());

            // Solicitar temperatura
            out.println("GET TEMP");
            System.out.println("Cliente: GET TEMP");
            System.out.println("Servidor: " + in.readLine());

            // Cerrar sesión
            out.println("QUIT");
            System.out.println("Cliente: QUIT");
            System.out.println("Servidor: " + in.readLine());

        } catch (IOException e) {
            System.out.println("Error: " + e.getMessage());
        }
    }
}
