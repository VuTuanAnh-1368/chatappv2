#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8000
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    int sockfd;
    struct sockaddr_in addr;

}
Client;

Client *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


void add_client(Client *client);

void remove_client(int sockfd);

void broadcast_message(const char *message, int sender_sockfd);

void *client_handler(void *arg);

int main() {
    int server_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("listen failed!");
        close(server_fd);
        return 1;
    }

    while(1) {
        Client *client = (Client *)malloc(sizeof(Client));
        
        socklen_t addr_len = sizeof(client->addr);
        
        client->sockfd = accept(server_fd, (struct sockaddr *)&client->addr, &addr_len);
        
        if (client->sockfd < 0) {
            free(client);
            continue;
        }
        
        int added = 0;
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == NULL) {
                added = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (!added) {
            const char *full_msg = "[Server] Room is full\n";
            send(client->sockfd, full_msg, strlen(full_msg), 0);
            close(client->sockfd);
            free(client);
            continue;
        }

        add_client(client);

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, client) != 0) {
            perror("pthread_create");
            remove_client(client->sockfd);
            close(client->sockfd);
            free(client);
            continue;
        }

        pthread_detach(tid);
    }
    
    close(server_fd);

    return 0;
}



void add_client(Client *client) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int sockfd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->sockfd == sockfd) {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}


void broadcast_message(const char *message, int sender_sockfd) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->sockfd != sender_sockfd) {
            if (send(clients[i]->sockfd, message, strlen(message), 0) < 0) {
                perror("send");
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *client_handler(void *arg) {
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + 64];
    int len;

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->addr.sin_addr), ip, INET_ADDRSTRLEN);
    int port = ntohs(client->addr.sin_port);

    snprintf(message, sizeof(message), "Cllient %s:%d joined\n", ip, port);
    printf("%s", message);
    broadcast_message(message, client->sockfd);

    while (1) {
        len = recv(client->sockfd, buffer, BUFFER_SIZE - 1, 0);

        if (len > 0) {
            buffer[len] = '\0';

            printf("[From %s:%d] %s", ip, port, buffer);

            snprintf(message, sizeof(message), "[%s:%d] %s", ip, port, buffer);
            broadcast_message(message, client->sockfd);

        } else if (len == 0) {
            printf("Client %s:%d disconnected\n", ip, port);
            snprintf(message, sizeof(message), "[Server] Client %s:%d left\n", ip, port);
            broadcast_message(message, client->sockfd);
            break;

        } else {
            perror("recv");
            printf("Client %s:%d error\n", ip, port);
            snprintf(message, sizeof(message), "[Server] Client %s:%d left\n", ip, port);
            broadcast_message(message, client->sockfd);
            break;
        }
    }

    close(client->sockfd);
    remove_client(client->sockfd);
    free(client);
    return NULL;
}
