
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 6001
#define MAX_CLIENTS 10

typedef struct {
    int client_socket;
    struct sockaddr_in client_address;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[1024];
    ssize_t bytes_received;

    // Receive username from client
    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0) {
        perror("Error receiving username");
        close(client_socket);
        return NULL;
    }
    buffer[bytes_received] = '\0';
    printf("Received username: %s\n", buffer);

    // Acknowledge username receipt
    send(client_socket, "Username received", strlen("Username received"), 0);

    // Chat logic - Example: send and receive messages
    while (1) {
        // Receive message from client
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            perror("Error receiving message");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("Received message from client: %s\n", buffer);

        // Send message to the other client
        for (int i = 0; i < num_clients; i++) {
            if (clients[i].client_socket != client_socket) {
                send(clients[i].client_socket, buffer, bytes_received, 0);
                break; // Only send to one client
            }
        }
    }

    close(client_socket);
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len;
    pthread_t tid;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind to port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Waiting for connections...\n");

    while (1) {
        // Accept a new connection
        client_address_len = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }

        // Add client info to the array
        pthread_mutex_lock(&mutex);
        if (num_clients < MAX_CLIENTS) {
            clients[num_clients].client_socket = client_socket;
            clients[num_clients].client_address = client_address;
            num_clients++;
            printf("Client connected. Total clients: %d\n", num_clients);

            // Create a new thread to handle client
            if (pthread_create(&tid, NULL, handle_client, &client_socket) != 0) {
                perror("Thread creation failed");
                close(client_socket);
            }
        } else {
            printf("Maximum clients reached. Connection rejected.\n");
            close(client_socket);
        }
        pthread_mutex_unlock(&mutex);
    }

    close(server_socket);
    return 0;
}
