#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <sys/epoll.h>
#include <signal.h>


#define PORT 8000
#define BUFFER_SIZE 1024
#define MAX_EVENTS 100
#define MAX_CLIENTS 100

int clients[MAX_CLIENTS];
void add_client(int fd);
void remove_client(int fd);
void broadcast_message(int sender_fd, const char *msg, int len);
void handle_sigint(int sig);

volatile int running = 1;

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char buffer[BUFFER_SIZE];
    int n;
    int epfd;
    struct epoll_event ev, events[MAX_EVENTS];
    
    signal(SIGINT, handle_sigint);
    memset(clients, -1, sizeof(clients));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
   
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }
    
    if (listen(server_fd, 5) < 0) {
        perror("listen failed!");
        close(server_fd);
        return 1;
    }

    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("Epoll_create1");
        close(server_fd);
        return 1;
    }
    
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
    

    while(running) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            perror("epoll_wait");
            continue;
        }
        
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            
            if (fd == server_fd) {  
                client_fd = accept(server_fd,(struct sockaddr *)&addr, &addr_len);
                
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
                
                add_client(client_fd);
                printf("new client: fd = %d\n",client_fd);  
            }
            else {
                n = recv(fd, buffer, BUFFER_SIZE - 1, 0);
                if (n <= 0) {
                    printf("Client fd = %d disconnect!\n", fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    remove_client(fd);
                    close(fd);
                }
                else {
                    buffer[n] = '\0';
                    printf("fd = %d : %s", fd, buffer);
                    broadcast_message(fd, buffer, n);
                }
            }
        }
        
    }
    


    printf("\nShutting down...\n");
    close(server_fd);
    close(epfd);

    return 0;
}


void add_client(int fd) {
    int i;
    for (i = 0; i < MAX_CLIENTS; i ++) {
        if (clients[i] == -1) {
            clients[i] = fd;
            return;
        }
    }    
}

void remove_client(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == fd) {
            clients[i] = -1;
            return;
        }
    }
}

void broadcast_message(int sender_fd, const char *msg, int len) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1 && clients[i] != sender_fd) {
            send(clients[i], msg, len, 0);
        }
    }
}


void handle_sigint(int sig) {
    running = 0;
}
