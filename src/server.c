/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Write a multithreaded TCP server in C that accepts connections and sinks data."
 */

#include "common.h"
#include <pthread.h>
#include <signal.h>

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    // Use the unified 'Message' struct now
    Message *my_msg = create_complex_message(1024);
    
    if (my_msg == NULL) {
        perror("Failed to allocate complex message");
        close(client_sock);
        return NULL;
    }

    char *recv_buffer = (char *)malloc(1024 * 1024);
    if (!recv_buffer) {
        perror("Failed to allocate recv buffer");
        free_complex_message(my_msg);
        close(client_sock);
        return NULL;
    }

    ssize_t bytes_read;
    while ((bytes_read = recv(client_sock, recv_buffer, 1024 * 1024, 0)) > 0) {
        // Drain buffer
    }

    free(recv_buffer);
    free_complex_message(my_msg);
    close(client_sock);
    return NULL;
}

int main() {
    int server_fd, *client_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    signal(SIGPIPE, SIG_IGN);

    setup_server_socket(&server_fd);
    printf("Server listening on port %d\n", SERVER_PORT);

    while (1) {
        client_sock = malloc(sizeof(int));
        if ((*client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            free(client_sock);
            continue;
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_sock) < 0) {
            perror("pthread_create");
            free(client_sock);
        } else {
            pthread_detach(thread_id);
        }
    }
    return 0;
}