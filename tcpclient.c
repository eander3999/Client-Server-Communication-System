#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Invalid args\n");
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int X = atoi(argv[3]);

    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Client: socket error");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Client: Invalid address\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Client: connect error");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    snprintf(buffer, BUFFER_SIZE, "HELLO %d", X);

    if (send(sock_fd, buffer, strlen(buffer), 0) != strlen(buffer)) {
        perror("Client: send error");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, BUFFER_SIZE);

    ssize_t n = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
    if (n <= 0) {
        perror("Client: recv error");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0';

    printf("%s\n", buffer);
    fflush(stdout);

    int Y;
    if (sscanf(buffer, "HELLO %d", &Y) != 1) {
        fprintf(stderr, "ERROR\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    if (Y != X + 1) {
        fprintf(stderr, "ERROR\n");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    int Z = Y + 1;
    snprintf(buffer, BUFFER_SIZE, "HELLO %d", Z);

    if (send(sock_fd, buffer, strlen(buffer), 0) != strlen(buffer)) {
        perror("Client: send error");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    close(sock_fd);
    return 0;
}
