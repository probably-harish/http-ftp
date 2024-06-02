// The following contains my implementation of a simple HTTP server which can handle the following classes of requests:
// GET
// POST
// PUT
// DELETE from a valid HTTP client
//
// The web resources that are referenced to the in the requests are handled by simulating a "web" directory inside which you shall put the necessary files. 
// Simple error handling mechanisms prevent one from accessing resources outside of the web directory. 
// Eiter create your own "web" directory and put it in the working directory, same as the server executable, or use the web directory found in the zip. 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 4096

// Function to decode URL for getting the actual file path
void url_decode(char *src, char *dest, int max) {
    char *p = src;
    char code[3] = {0};
    while (*p && --max) {
        if (*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char)strtol(code, NULL, 16);
            p += 2;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

// Function to handle the GET requests from the clinet
void handle_get(int client_socket, char *resource) {
    char filepath[BUFFER_SIZE];
    url_decode(resource, filepath, sizeof(filepath));
    
    if (strstr(filepath, "..")) {
        char response[] = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain\r\n\r\nForbidden: Resource access is forbidden.\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
        return;
    }
    
    if (strcmp(filepath, "/") == 0) {
        strcat(filepath, "index.html");
    }
    
    char fullpath[BUFFER_SIZE] = "./web";
    strcat(fullpath, filepath);

    FILE *file = fopen(fullpath, "rb");  // bin mode
    if (file == NULL) {
        char response[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found: The requested resource was not found on this server.\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
        return;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);


    char *content_type = "Content-Type: text/plain\r\n";  // Default to text/plain for this example
    if (strstr(filepath, ".html")) {
        content_type = "Content-Type: text/html\r\n";
    } else if (strstr(filepath, ".jpg") || strstr(filepath, ".jpeg")) {
        content_type = "Content-Type: image/jpeg\r\n";
    } else if (strstr(filepath, ".png")) {
        content_type = "Content-Type: image/png\r\n";
    } else if (strstr(filepath, ".css")) {
        content_type = "Content-Type: text/css\r\n";
    } else if (strstr(filepath, ".js")) {
        content_type = "Content-Type: application/javascript\r\n";
    }

    // Send header
    char response_header[200];
    sprintf(response_header, "HTTP/1.1 200 OK\r\n%sContent-Length: %ld\r\n\r\n", content_type, fsize);
    send(client_socket, response_header, strlen(response_header), 0);

    // Send file content
    char file_buffer[BUFFER_SIZE];
    size_t read_size;
    while ((read_size = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, file_buffer, read_size, 0);
    }

    fclose(file);
}

// Put function 
void handle_put(int client_socket, char *resource, char *data) {
    char filepath[BUFFER_SIZE];
    url_decode(resource, filepath, sizeof(filepath));

    if (strstr(filepath, "..")) {
        char response[] = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain\r\n\r\nForbidden: Resource access is forbidden.\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
        return;
    }

    char fullpath[BUFFER_SIZE] = "./web";
    strcat(fullpath, filepath);

    FILE *file = fopen(fullpath, "wb"); // Open file in binary mode
    if (file == NULL) {
        char response[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\n500 Internal Server Error: Could not open file for writing.\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
        return;
    }

    size_t data_len = strlen(data);
    fwrite(data, sizeof(char), data_len, file);
    fclose(file);

    char response[] = "HTTP/1.1 201 Created\r\nContent-Type: text/plain\r\n\r\n201 Created: File saved successfully.\r\n";
    send(client_socket, response, sizeof(response) - 1, 0);
}

// Post function 
void handle_post(int client_socket, char *resource, char *data) {
    char filepath[BUFFER_SIZE];
    url_decode(resource, filepath, sizeof(filepath));

    if (strstr(filepath, "..")) {
        char response[] = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain\r\n\r\nForbidden: Resource access is forbidden.\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
        return;
    }

    char fullpath[BUFFER_SIZE] = "./web";
    strcat(fullpath, filepath);

    // We're appending data to the file
    FILE *file = fopen(fullpath, "ab"); 
    if (file == NULL) {
        char response[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\n500 Internal Server Error: Could not open file for appending.\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
        return;
    }

    size_t data_len = strlen(data);
    fwrite(data, sizeof(char), data_len, file);
    fclose(file);

    char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n200 OK: Data appended successfully.\r\n";
    send(client_socket, response, sizeof(response) - 1, 0);
}

// Delete function 
void handle_delete(int client_socket, char *resource) {
    char filepath[BUFFER_SIZE];
    url_decode(resource, filepath, sizeof(filepath));

    if (strstr(filepath, "..")) {
        char response[] = "HTTP/1.1 403 Forbidden\r\nContent-Type: text/plain\r\n\r\nForbidden: Resource access is forbidden.\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
        return;
    }

    char fullpath[BUFFER_SIZE] = "./web";
    strcat(fullpath, filepath);

    if (remove(fullpath) == 0) {
        char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n200 OK: File deleted successfully.\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
    } else {
        char response[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found: The requested resource was not found on this server.\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
    }
}

void handle_request(int client_socket, char *request) {
    char method[5], resource[BUFFER_SIZE], *data;
    sscanf(request, "%4s %s", method, resource);
    data = strstr(request, "\r\n\r\n"); 
    if (data) data += 4; 

    if (strncmp(method, "GET", 3) == 0) {
        handle_get(client_socket, resource);
    } else if (strncmp(method, "PUT", 3) == 0) {
        if (data) handle_put(client_socket, resource, data);
    } else if (strncmp(method, "POST", 4) == 0) {
        if (data) handle_post(client_socket, resource, data);
    } else if (strncmp(method, "DELETE", 6) == 0) {
        handle_delete(client_socket, resource);
    } else {
        char response[] = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/plain\r\n\r\nNot Implemented\r\n";
        send(client_socket, response, sizeof(response) - 1, 0);
    }
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t addr_size = sizeof(client_address);
    char buffer[BUFFER_SIZE];

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    int option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addr_size);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        int received_bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (received_bytes < 0) {
            perror("Recv failed");
            close(client_socket);
            continue;
        } else if (received_bytes == 0) {
            printf("Client disconnected unexpectedly.\n");
            close(client_socket);
            continue;
        } else {
            buffer[received_bytes] = '\0';  // NULL-terminate the buffer
            handle_request(client_socket, buffer);
        }

        close(client_socket);
    }

    close(server_socket);
    return 0;
}
