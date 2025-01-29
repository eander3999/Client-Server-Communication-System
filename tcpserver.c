#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUFFER_SIZE 256

typedef enum {
    STATE_FIRST_SHAKE,
    STATE_SECOND_SHAKE
} client_state_t;

typedef struct {
    int fd;
    client_state_t state;
    int X;
    int Y;
} client_t;

void handle_sigint(int sig) {
    printf("\nExiting\n");
    exit(0);
}

void handle_first_shake(client_t *client);
void handle_second_shake(client_t *client);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Invalid args\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_sigint);

    int server_port = atoi(argv[1]);
    int listen_fd;
    struct sockaddr_in server_addr;

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt error");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen error");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    client_t clients[100];
    for (int i = 0; i < 100; i++) {
        clients[i].fd = -1;
    }

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        FD_SET(listen_fd, &read_fds);
        int max_fd = listen_fd;

        for (int i = 0; i < 100; i++) {
            int fd = clients[i].fd;
            if (fd != -1) {
                FD_SET(fd, &read_fds);
                if (fd > max_fd) {
                    max_fd = fd;
                }
            }
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(listen_fd, &read_fds)) {
            int new_socket;
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            if ((new_socket = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
                perror("accept error");
                continue;
            }

            int added = 0;
            for (int i = 0; i < 100; i++) {
                if (clients[i].fd == -1) {
                    clients[i].fd = new_socket;
                    clients[i].state = STATE_FIRST_SHAKE;
                    clients[i].X = 0;
                    clients[i].Y = 0;
                    added = 1;
                    break;
                }
            }

            if (!added) {
                close(new_socket);
            }
        }

        for (int i = 0; i < 100; i++) {
            int fd = clients[i].fd;
            if (fd != -1 && FD_ISSET(fd, &read_fds)) {
                if (clients[i].state == STATE_FIRST_SHAKE) {
                    handle_first_shake(&clients[i]);
                } else if (clients[i].state == STATE_SECOND_SHAKE) {
                    handle_second_shake(&clients[i]);
                } else {
                    close(fd);
                    clients[i].fd = -1;
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}

void handle_first_shake(client_t *client) {
    char buffer[BUFFER_SIZE];
    char output_buffer[BUFFER_SIZE * 2];
    ssize_t n = recv(client->fd, buffer, BUFFER_SIZE - 1, 0);
    if (n <= 0) {
        if (n < 0) perror("recv error");
        close(client->fd);
        client->fd = -1;
        return;
    }
    buffer[n] = '\0';

    for (ssize_t i = 0; i < n; i++) {
        if (buffer[i] == '\r') {
            buffer[i] = '\0';
            n = i;
            break;
        }
    }

    int output_len = snprintf(output_buffer, sizeof(output_buffer), "%s\n", buffer);
    write(STDOUT_FILENO, output_buffer, output_len);

    int X;
    if (sscanf(buffer, "HELLO %d", &X) != 1) {
        fprintf(stderr, "ERROR\n");
        close(client->fd);
        client->fd = -1;
        return;
    }

    client->X = X;
    client->Y = X + 1;

    char response[BUFFER_SIZE];
    int response_len = snprintf(response, BUFFER_SIZE, "HELLO %d", client->Y);

    if (send(client->fd, response, response_len, 0) != response_len) {
        perror("send error");
        close(client->fd);
        client->fd = -1;
        return;
    }

    client->state = STATE_SECOND_SHAKE;
}

void handle_second_shake(client_t *client) {
    char buffer[BUFFER_SIZE];
    char output_buffer[BUFFER_SIZE * 2];
    ssize_t n = recv(client->fd, buffer, BUFFER_SIZE - 1, 0);
    if (n <= 0) {
        if (n < 0) perror("recv error");
        close(client->fd);
        client->fd = -1;
        return;
    }
    buffer[n] = '\0';

    for (ssize_t i = 0; i < n; i++) {
        if (buffer[i] == '\r') {
            buffer[i] = '\0';
            n = i;
            break;
        }
    }

    int output_len = snprintf(output_buffer, sizeof(output_buffer), "%s\n", buffer);
    write(STDOUT_FILENO, output_buffer, output_len);

    int Z;
    if (sscanf(buffer, "HELLO %d", &Z) != 1) {
        fprintf(stderr, "ERROR\n");
        close(client->fd);
        client->fd = -1;
        return;
    }

    if (Z != client->Y + 1) {
        fprintf(stderr, "ERROR\n");
    }

    close(client->fd);
    client->fd = -1;
}
