#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8000
#define BUFFER_SIZE 1024


int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char buffer[BUFFER_SIZE];
    int n;

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
    
    if (listen(server_fd, 5) < 0) {
        perror("listen failed!");
        close(server_fd);
        return 1;
    }

    

    client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (n > 0) {
            buffer[n] = '\0';
            printf("Client: %s", buffer);
            send(client_fd, buffer, strlen(buffer), 0);
        }
        else if (n == 0) { 
            printf("Client disconnect..!");
            break;    
        }
        else {
            perror("Recv");
            break;
        }
    }
    
    close(client_fd);
    close(server_fd);

    return 0;
}
