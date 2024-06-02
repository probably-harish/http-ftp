#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 4096

void send_http_request(int socket, const char *method, const char *path, const char *data) {
    char buffer[BUFFER_SIZE];
    if (strcmp(method, "GET") == 0 || strcmp(method, "DELETE") == 0) {
        snprintf(buffer, sizeof(buffer), "%s %s HTTP/1.1\r\nHost: localhost:%d\r\n\r\n", method, path, SERVER_PORT);
    } else { // PUT or POST
        snprintf(buffer, sizeof(buffer), "%s %s HTTP/1.1\r\nHost: localhost:%d\r\nContent-Length: %ld\r\n\r\n%s", method, path, SERVER_PORT, strlen(data), data);
    }

    send(socket, buffer, strlen(buffer), 0);
    printf("Sent HTTP Request:\n%s", buffer);
}

int main() {
    char method[10], path[256], data[BUFFER_SIZE], buffer[BUFFER_SIZE];
    int client_socket;
    struct hostent *server;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to the server on port %d...\n", SERVER_PORT);

    while (1) {
    // Get the request method from the user
    printf("Enter HTTP method (GET, POST, PUT, DELETE): ");
    fgets(method, sizeof(method), stdin);
    method[strcspn(method, "\n")] = 0; // Remove newline at the end

    // Get the path/resource from the user
    printf("Enter path (e.g., /index.html): ");
    fgets(path, sizeof(path), stdin);
    path[strcspn(path, "\n")] = 0;

    // If method is POST or PUT, get data from the user
    if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) {
        printf("Enter data (JSON or URL-encoded form): ");
        fgets(data, sizeof(data), stdin);
        data[strcspn(data, "\n")] = 0;
    } else {
        strcpy(data, ""); // No data needed for GET or DELETE
    }

    send_http_request(client_socket, method, path, data);

    // Receive HTTP response
    int received_bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

    if (received_bytes < 0) {
        perror("Recv failed");
    } else {
        buffer[received_bytes] = '\0'; // Null-terminate the received data
        printf("Received response from the server:\n%s", buffer);
	}
    }
    close(client_socket);
    return 0;
}
