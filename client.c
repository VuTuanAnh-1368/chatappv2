#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8000
#define BUFFER_SIZE 1024

void *receive_thread(void *arg);
void *send_thread(void *arg);


int main() {
    struct sockaddr_in server_addr;
    pthread_t t_recv, t_send;
    int sockfd;

    printf("Start");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socker failed!");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);


    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failde!");
        close(sockfd);
        return 1;
    }
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connet failed");
        close(sockfd);
        return 1;
    }   

    printf("Connected to server!!!");
    pthread_create(&t_recv, NULL, receive_thread, &sockfd);
    pthread_create(&t_send, NULL, send_thread, &sockfd);

    pthread_join(t_send, NULL);
    pthread_join(t_recv, NULL);

    close(sockfd);
    printf("Client exited!!!\n");
    
    return 0;
}


void *receive_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    int len;
    int sock_fd = *(int *)arg;

    while(1) {
        len = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            printf("%s", buffer);
        }
        else if (len == 0) {
            printf("Server disconnect!!\n");
            break;
        }
        else {
            printf("recv");
            break;
        }
    }
    return NULL;

}


void *send_thread(void *arg) {
    
    int sock_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while(1) {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Input closed");
            break;
        }
           

        if (strcmp(buffer, "/quit\n") == 0 ) {
            printf("Disconnecting..\n");
            shutdown(sock_fd, SHUT_RDWR);
            break;
        }
        int len = strlen(buffer);
        int sent = send(sock_fd, buffer, len, 0);

        if (sent < 0) {
            perror("Send faileeee");
            break;
        }
    }

    return NULL;
        
}
