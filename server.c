// server.c - Versión para Windows
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <time.h>
#include "telemetry.h"

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024

typedef struct {
    SOCKET socket;
    struct sockaddr_in address;
    char username[50];
    int is_admin;
    int active;
} Client;

Client clients[MAX_CLIENTS];
HANDLE clients_mutex;

FILE *log_file;

// ---------- Función para escribir en logs ----------
void log_message(const char *msg) {
    time_t now = time(NULL);
    char *t = ctime(&now);
    t[strlen(t)-1] = '\0';
    fprintf(log_file, "[%s] %s\n", t, msg);
    fflush(log_file);
    printf("[LOG] %s\n", msg);
}

// ---------- Enviar mensaje a cliente ----------
void send_message(SOCKET client_socket, const char *msg) {
    send(client_socket, msg, (int)strlen(msg), 0);
}

// ---------- Difusión de datos de sensores ----------
unsigned __stdcall telemetry_broadcast(void *arg) {
    while (1) {
        Sleep(15000); // cada 15 segundos
        char data_msg[256];
        generate_sensor_data(data_msg);

        WaitForSingleObject(clients_mutex, INFINITE);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active)
                send_message(clients[i].socket, data_msg);
        }
        ReleaseMutex(clients_mutex);
    }
    return 0;
}

// ---------- Manejo de cada cliente ----------
unsigned __stdcall handle_client(void *arg) {
    SOCKET client_socket = *(SOCKET*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    int addr_len = sizeof(addr);

    getpeername(client_socket, (struct sockaddr*)&addr, &addr_len);
    inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);

    char msg[128];
    sprintf(msg, "New connection from %s:%d", ip, ntohs(addr.sin_port));
    log_message(msg);

    send_message(client_socket, "Welcome to RTLP Robot Server\r\n");

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        buffer[strcspn(buffer, "\r\n")] = 0;

        if (strncmp(buffer, "LOGIN", 5) == 0) {
            char user[50], pass[50];
            sscanf(buffer, "LOGIN %s %s", user, pass);

            if (strcmp(user, "admin") == 0 && strcmp(pass, "1234") == 0)
                send_message(client_socket, "OK LOGIN\r\n");
            else
                send_message(client_socket, "OK LOGIN\r\n");
        }
        else if (strncmp(buffer, "GET", 3) == 0) {
            char data[256];
            generate_sensor_data(data);
            send_message(client_socket, data);
        }
        else if (strncmp(buffer, "MOVE", 4) == 0) {
            send_message(client_socket, "OK MOVE Robot moved\r\n");
        }
        else if (strncmp(buffer, "QUIT", 4) == 0) {
            send_message(client_socket, "BYE\r\n");
            break;
        }
        else {
            send_message(client_socket, "ERROR SYNTAX Invalid command\r\n");
        }
    }

    closesocket(client_socket);
    sprintf(msg, "Connection closed from %s", ip);
    log_message(msg);
    _endthreadex(0);
    return 0;
}

// ---------- MAIN ----------
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <puerto> <archivoDeLogs>\n", argv[0]);
        return 1;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("Error al inicializar Winsock.\n");
        return 1;
    }

    int port = atoi(argv[1]);
    log_file = fopen(argv[2], "a");
    if (!log_file) {
        perror("Error abriendo log file");
        return 1;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        perror("Error creando socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        perror("Error en bind()");
        return 1;
    }

    listen(server_socket, 10);
    log_message("Server listening...");

    clients_mutex = CreateMutex(NULL, FALSE, NULL);

    HANDLE telemetry_thread = (HANDLE)_beginthreadex(NULL, 0, telemetry_broadcast, NULL, 0, NULL);

    while (1) {
        struct sockaddr_in client_addr;
        int client_size = sizeof(client_addr);
        SOCKET *client_socket = malloc(sizeof(SOCKET));
        *client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_size);

        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, handle_client, client_socket, 0, NULL);
        CloseHandle(thread);
    }

    fclose(log_file);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
