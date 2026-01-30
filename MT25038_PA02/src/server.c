/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Write a multithreaded TCP server in C that accepts connections and sinks data."
 */

#include "common.h"
#include <pthread.h>

#define RECV_BUFFER_SIZE 65536

void *handle_client(void *socket_desc) {
    int sock = *(int*)socket_desc;
    free(socket_desc);

    char *buffer = malloc(RECV_BUFFER_SIZE);
    if (!buffer) {
        close(sock);
        return NULL;
    }

    ssize_t bytes_read;
    long total_bytes = 0;

    while ((bytes_read = recv(sock, buffer, RECV_BUFFER_SIZE, 0)) > 0) {
        total_bytes += bytes_read;
    }

    free(buffer);
    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int port = SERVER_PORT;

    if (argc > 1) port = atoi(argv[1]);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) exit(EXIT_FAILURE);
    if (listen(server_fd, 10) < 0) exit(EXIT_FAILURE);

    printf("Server listening on %d\n", port);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) continue;
        
        int *new_sock_ptr = malloc(sizeof(int));
        *new_sock_ptr = new_socket;
        
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void*)new_sock_ptr);
        pthread_detach(thread_id);
    }
    return 0;
}