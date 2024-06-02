#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <dirent.h>

#define SERVER_PORT 21
#define DATA_PORT 2021 
#define BUFFER_SIZE 4096
#define WEB_DIR "./web"

int create_data_socket();

void send_list(int data_sock) {
    DIR *d;
    struct dirent *dir;
    d = opendir(WEB_DIR);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char buffer[256];
            sprintf(buffer, "%s\n", dir->d_name);
            send(data_sock, buffer, strlen(buffer), 0);
        }
        closedir(d);
    }
    close(data_sock);
}

void send_file(int data_sock, const char *filename) {
    char file_path[BUFFER_SIZE];
    snprintf(file_path, BUFFER_SIZE, "%s/%s", WEB_DIR, filename);

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        close(data_sock);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(data_sock, buffer, bytes_read, 0);
    }
    fclose(file);
    close(data_sock);
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    send(client_sock, "220 (ftp_server 1.0)\r\n", 22, 0);

    while (recv(client_sock, buffer, BUFFER_SIZE, 0) > 0) {
        printf("Received: %s", buffer);
        
        if (strncmp(buffer, "USER", 4) == 0 || strncmp(buffer, "PASS", 4) == 0) {
            send(client_sock, "230 Login successful.\r\n", 23, 0);
        } else if (strncmp(buffer, "LIST", 4) == 0) {
            send(client_sock, "150 Here comes the directory listing.\r\n", 39, 0);
            int data_sock = create_data_socket();
            send_list(data_sock);
	    send(client_sock, "\r\n", 2, 0); 
            send(client_sock, "226 Directory send OK.\r\n", 24, 0);
        } else if (strncmp(buffer, "RETR", 4) == 0) {
            char filename[BUFFER_SIZE];
            sscanf(buffer + 5, "%s", filename);  
            send(client_sock, "150 Opening BINARY mode data connection.\r\n", 40, 0);
            int data_sock = create_data_socket();
            send_file(data_sock, filename);
            send(client_sock, "226 File send OK.\r\n", 19, 0);
        } else if (strncmp(buffer, "QUIT", 4) == 0) {
            send(client_sock, "221 Goodbye.\r\n", 14, 0);
            break;
        }

        memset(buffer, 0, BUFFER_SIZE); // Clear buffer for next command
    }

    close(client_sock);
}

int create_data_socket() {
    int sock;
    struct sockaddr_in server_addr;
    int opt = 1; // Option value for SO_REUSEADDR

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Cannot create socket");
        exit(1);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(sock);
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DATA_PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Cannot bind to port");
        close(sock);
        exit(1);
    }

    if (listen(sock, 1) < 0) {
        perror("Cannot listen on port");
        close(sock);
        exit(1);
    }

    int data_sock = accept(sock, NULL, NULL);
    close(sock);
    return data_sock;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int opt = 1;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Cannot create socket");
        return 1;
    }

    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(server_sock);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Cannot bind socket");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 10) < 0) {
        perror("Cannot listen on socket");
        close(server_sock);
        return 1;
    }

    printf("FTP Server started on port %d. Data port: %d\n", SERVER_PORT, DATA_PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addrlen);
        if (client_sock < 0) {
            perror("Cannot accept connection");
            continue;
        }

        if (!fork()) { // Child process
            close(server_sock); // Close listening socket in the child process
            handle_client(client_sock);
            exit(0);
        }
        close(client_sock); 
    }

    close(server_sock);
    return 0;
}
