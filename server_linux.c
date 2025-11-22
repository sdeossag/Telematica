// server_linux.c - Versión para Linux/Ubuntu
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>

#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    struct sockaddr_in address;
    char username[50];
    int is_admin;
    int active;
    int x;  // Posición X del robot
    int y;  // Posición Y del robot
} Client;

typedef struct {
    float temp;
    float hum;
    float pres;
    float co2;
    int robot_x;
    int robot_y;
} RobotState;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
RobotState current_state = {20.0, 50.0, 1010.0, 420.0, 5, 5};
pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE *log_file;
int server_running = 1;

// ---------- Función para escribir en logs ----------
void log_message(const char *msg) {
    time_t now = time(NULL);
    char *t = ctime(&now);
    t[strlen(t)-1] = '\0';
    
    pthread_mutex_lock(&clients_mutex);
    fprintf(log_file, "[%s] %s\n", t, msg);
    fflush(log_file);
    printf("[LOG] %s\n", msg);
    pthread_mutex_unlock(&clients_mutex);
}

// ---------- Generar datos de sensores ----------
void generate_sensor_data(char *buffer) {
    pthread_mutex_lock(&state_mutex);
    
    // Simular variación de sensores
    current_state.temp = 20 + (rand() % 100) / 10.0;
    current_state.hum = 40 + (rand() % 300) / 10.0;
    current_state.pres = 1000 + (rand() % 200) / 10.0;
    current_state.co2 = 400 + (rand() % 500) / 10.0;
    
    sprintf(buffer, "DATA TEMP=%.1f HUM=%.1f PRES=%.1f CO2=%.1f X=%d Y=%d\r\n",
            current_state.temp, current_state.hum, current_state.pres, 
            current_state.co2, current_state.robot_x, current_state.robot_y);
    
    pthread_mutex_unlock(&state_mutex);
}

// ---------- Enviar mensaje a cliente ----------
void send_message(int client_socket, const char *msg) {
    send(client_socket, msg, strlen(msg), 0);
}

// ---------- Broadcast de telemetría ----------
void *telemetry_broadcast(void *arg) {
    while (server_running) {
        sleep(5);  // Cada 5 segundos
        char data_msg[256];
        generate_sensor_data(data_msg);

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                send_message(clients[i].socket, data_msg);
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        
        log_message("Telemetry broadcast sent");
    }
    return NULL;
}

// ---------- Buscar slot libre para cliente ----------
int find_client_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active)
            return i;
    }
    return -1;
}

// ---------- Mover robot ----------
void move_robot(const char *direction) {
    pthread_mutex_lock(&state_mutex);
    
    if (strcmp(direction, "UP") == 0 && current_state.robot_y > 0)
        current_state.robot_y--;
    else if (strcmp(direction, "DOWN") == 0 && current_state.robot_y < 10)
        current_state.robot_y++;
    else if (strcmp(direction, "LEFT") == 0 && current_state.robot_x > 0)
        current_state.robot_x--;
    else if (strcmp(direction, "RIGHT") == 0 && current_state.robot_x < 10)
        current_state.robot_x++;
    
    pthread_mutex_unlock(&state_mutex);
}

// ---------- Manejo de cada cliente ----------
void *handle_client(void *arg) {
    int client_index = *(int*)arg;
    free(arg);
    
    int client_socket = clients[client_index].socket;
    char buffer[BUFFER_SIZE];
    char ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &clients[client_index].address.sin_addr, ip, INET_ADDRSTRLEN);

    char msg[256];
    sprintf(msg, "New connection from %s:%d", ip, ntohs(clients[client_index].address.sin_port));
    log_message(msg);

    send_message(client_socket, "Welcome to RTLP Robot Server v2.0\r\n");

    while (server_running) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes <= 0) break;

        buffer[strcspn(buffer, "\r\n")] = 0;
        
        sprintf(msg, "Command from %s: %s", ip, buffer);
        log_message(msg);

        if (strncmp(buffer, "LOGIN", 5) == 0) {
            char user[50], pass[50];
            if (sscanf(buffer, "LOGIN %s %s", user, pass) == 2) {
                strcpy(clients[client_index].username, user);
                
                if (strcmp(user, "admin") == 0 && strcmp(pass, "1234") == 0) {
                    clients[client_index].is_admin = 1;
                    send_message(client_socket, "OK LOGIN admin\r\n");
                    log_message("Admin login successful");
                } else {
                    clients[client_index].is_admin = 0;
                    send_message(client_socket, "OK LOGIN user\r\n");
                    log_message("User login successful");
                }
            } else {
                send_message(client_socket, "ERROR SYNTAX Invalid LOGIN format\r\n");
            }
        }
        else if (strncmp(buffer, "GET", 3) == 0) {
            char data[256];
            generate_sensor_data(data);
            send_message(client_socket, data);
        }
        else if (strncmp(buffer, "MOVE", 4) == 0) {
            if (!clients[client_index].is_admin) {
                send_message(client_socket, "ERROR PERMISSION Only admin can move robot\r\n");
            } else {
                char direction[10];
                if (sscanf(buffer, "MOVE %s", direction) == 1) {
                    move_robot(direction);
                    
                    char response[256];
                    sprintf(response, "OK MOVE Robot moved %s to (%d,%d)\r\n", 
                            direction, current_state.robot_x, current_state.robot_y);
                    send_message(client_socket, response);
                    
                    sprintf(msg, "Robot moved %s by %s", direction, clients[client_index].username);
                    log_message(msg);
                } else {
                    send_message(client_socket, "ERROR SYNTAX Use: MOVE UP|DOWN|LEFT|RIGHT\r\n");
                }
            }
        }
        else if (strncmp(buffer, "STATUS", 6) == 0) {
            char status[512];
            pthread_mutex_lock(&state_mutex);
            sprintf(status, "STATUS X=%d Y=%d TEMP=%.1f HUM=%.1f PRES=%.1f CO2=%.1f\r\n",
                    current_state.robot_x, current_state.robot_y,
                    current_state.temp, current_state.hum, 
                    current_state.pres, current_state.co2);
            pthread_mutex_unlock(&state_mutex);
            send_message(client_socket, status);
        }
        else if (strncmp(buffer, "QUIT", 4) == 0) {
            send_message(client_socket, "BYE\r\n");
            break;
        }
        else {
            send_message(client_socket, "ERROR SYNTAX Invalid command. Use: LOGIN, GET, MOVE, STATUS, QUIT\r\n");
        }
    }

    close(client_socket);
    
    pthread_mutex_lock(&clients_mutex);
    clients[client_index].active = 0;
    pthread_mutex_unlock(&clients_mutex);
    
    sprintf(msg, "Connection closed from %s", ip);
    log_message(msg);
    
    return NULL;
}

// ---------- Manejador de señales ----------
void signal_handler(int sig) {
    printf("\nShutting down server...\n");
    server_running = 0;
}

// ---------- MAIN ----------
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <logfile>\n", argv[0]);
        printf("Example: %s 8080 server.log\n", argv[0]);
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    srand(time(NULL));

    int port = atoi(argv[1]);
    log_file = fopen(argv[2], "a");
    if (!log_file) {
        perror("Error opening log file");
        return 1;
    }

    // Inicializar array de clientes
    memset(clients, 0, sizeof(clients));

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        return 1;
    }

    // Permitir reutilizar el puerto
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error in bind()");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, 10) < 0) {
        perror("Error in listen()");
        close(server_socket);
        return 1;
    }

    char start_msg[256];
    sprintf(start_msg, "RTLP Server started on port %d", port);
    log_message(start_msg);
    printf("Server listening on port %d...\n", port);
    printf("Press Ctrl+C to stop\n\n");

    // Iniciar thread de telemetría
    pthread_t telemetry_thread;
    pthread_create(&telemetry_thread, NULL, telemetry_broadcast, NULL);

    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_size);

        if (client_socket < 0) {
            if (server_running)
                perror("Error in accept()");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        int slot = find_client_slot();
        if (slot >= 0) {
            clients[slot].socket = client_socket;
            clients[slot].address = client_addr;
            clients[slot].active = 1;
            clients[slot].is_admin = 0;
            
            int *arg = malloc(sizeof(int));
            *arg = slot;
            
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, arg);
            pthread_detach(thread);
        } else {
            send_message(client_socket, "ERROR SERVER Server full\r\n");
            close(client_socket);
            log_message("Connection rejected: server full");
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    log_message("Server shutting down");
    
    fclose(log_file);
    close(server_socket);
    
    printf("Server stopped.\n");
    return 0;
}