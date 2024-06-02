#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 21
#define DATA_PORT 2021 // Update this to the server's data port, if it's different
#define BUFFER_SIZE 1024

void ftp_list(int control_sock, int data_sock) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(data_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        write(1, buffer, bytes_received);
    }
}

void ftp_retr(int control_sock, int data_sock, char *filename) {
    char buffer[BUFFER_SIZE];
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    int bytes_received;
    while ((bytes_received = recv(data_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        write(file_fd, buffer, bytes_received);
    }
    close(file_fd);
}

int main(int argc, char *argv[]) {
    int control_sock, data_sock;
    struct sockaddr_in server_addr;

    if (argc < 3) {
        fprintf(stderr, "Usage: ftp_client <server-ip> <command> [filename]\n");
        exit(1);
    }

    control_sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if (connect(control_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Control connection failed");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    recv(control_sock, buffer, sizeof(buffer), 0);
    printf("%s", buffer);

    send(control_sock, "USER anonymous\r\n", 16, 0);
    recv(control_sock, buffer, sizeof(buffer), 0); // Wait for response
    printf("%s", buffer);

    send(control_sock, "PASS anonymous\r\n", 16, 0);
    recv(control_sock, buffer, sizeof(buffer), 0); // Wait for response
    printf("%s", buffer);

    if (strcmp(argv[2], "LIST") == 0) {
        send(control_sock, "LIST\r\n", 6, 0);
        recv(control_sock, buffer, sizeof(buffer), 0); // Wait for response
        printf("%s", buffer);

        data_sock = socket(AF_INET, SOCK_STREAM, 0);
        server_addr.sin_port = htons(DATA_PORT);
        if (connect(data_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Data connection failed");
            exit(1);
        }
        ftp_list(control_sock, data_sock);
        close(data_sock);

        recv(control_sock, buffer, sizeof(buffer), 0);
        printf("%s", buffer);
    } else if (strcmp(argv[2], "RETR") == 0 && argc == 4) {
        snprintf(buffer, sizeof(buffer), "RETR %s\r\n", argv[3]);
        send(control_sock, buffer, strlen(buffer), 0);
        recv(control_sock, buffer, sizeof(buffer), 0);
        printf("%s", buffer);

        data_sock = socket(AF_INET, SOCK_STREAM, 0);
        server_addr.sin_port = htons(DATA_PORT);
        if (connect(data_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Data connection failed");
            exit(1);
        }
        ftp_retr(control_sock, data_sock, argv[3]);
        close(data_sock);

        recv(control_sock, buffer, sizeof(buffer), 0);
        printf("%s", buffer);
    }

    send(control_sock, "QUIT\r\n", 6, 0);
    recv(control_sock, buffer, sizeof(buffer), 0);
    printf("%s", buffer);

    close(control_sock);
    return 0;
}
