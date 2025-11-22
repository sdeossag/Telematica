import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;

public class ClientGUI extends JFrame {
    private Socket socket;
    private BufferedReader in;
    private PrintWriter out;

    private JLabel lblTemp = new JLabel("TEMP: -- °C");
    private JLabel lblHum = new JLabel("HUM: -- %");
    private JLabel lblPres = new JLabel("PRES: -- hPa");
    private JLabel lblCO2 = new JLabel("CO2: -- ppm");
    private JPanel robotPanel;
    private int x = 90, y = 90;

    public ClientGUI() {
        setTitle("RTLP Client Java");
        setSize(300, 350);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLayout(new BorderLayout());

        // Panel principal
        JPanel infoPanel = new JPanel(new GridLayout(4, 1));
        infoPanel.add(lblTemp);
        infoPanel.add(lblHum);
        infoPanel.add(lblPres);
        infoPanel.add(lblCO2);
        add(infoPanel, BorderLayout.SOUTH);

        // Panel del robot
        robotPanel = new JPanel() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                g.setColor(Color.RED);
                g.fillOval(x, y, 20, 20);
            }
        };
        robotPanel.setBackground(Color.WHITE);
        add(robotPanel, BorderLayout.CENTER);

        // Conectar y login
        conectarServidor();
        recibirDatos();

        // Teclas para mover el robot
        addKeyListener(new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent e) {
                switch (e.getKeyCode()) {
                    case KeyEvent.VK_UP -> mover("UP", 0, -10);
                    case KeyEvent.VK_DOWN -> mover("DOWN", 0, 10);
                    case KeyEvent.VK_LEFT -> mover("LEFT", -10, 0);
                    case KeyEvent.VK_RIGHT -> mover("RIGHT", 10, 0);
                }
            }
        });

        setFocusable(true);
        setVisible(true);
    }

    private void conectarServidor() {
        try {
            socket = new Socket("100.24.14.26", 8080);
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            out = new PrintWriter(socket.getOutputStream(), true);

            System.out.println("Servidor: " + in.readLine());
            out.println("LOGIN admin 1234");
            System.out.println("Servidor: " + in.readLine());
        } catch (IOException e) {
            JOptionPane.showMessageDialog(this, "No se pudo conectar con el servidor.");
            System.exit(0);
        }
    }

    private void recibirDatos() {
        Thread hilo = new Thread(() -> {
            try {
                String msg;
                while ((msg = in.readLine()) != null) {
                    System.out.println("Servidor: " + msg);
                    if (msg.startsWith("DATA")) {
                        actualizarDatos(msg);
                    }
                }
            } catch (IOException e) {
                System.out.println("Conexión cerrada.");
            }
        });
        hilo.start();
    }

    private void actualizarDatos(String data) {
        String[] parts = data.split(" ");
        for (String part : parts) {
            if (part.contains("=")) {
                String[] kv = part.split("=");
                switch (kv[0]) {
                    case "TEMP" -> lblTemp.setText("TEMP: " + kv[1] + " °C");
                    case "HUM" -> lblHum.setText("HUM: " + kv[1] + " %");
                    case "PRES" -> lblPres.setText("PRES: " + kv[1] + " hPa");
                    case "CO2" -> lblCO2.setText("CO2: " + kv[1] + " ppm");
                }
            }
        }
    }

    private void mover(String dir, int dx, int dy) {
        out.println("MOVE " + dir);
        x += dx;
        y += dy;
        robotPanel.repaint();
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(ClientGUI::new);
    }
}
