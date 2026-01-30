/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Write a C TCP client using MSG_ZEROCOPY and process the ERRQUEUE to confirm completion."
 */

#include "common.h"
#include <pthread.h>

#define DURATION_SECONDS 10
#define STRING_LEN 4096 

typedef struct {
    char *server_ip;
    int port;
} ThreadArgs;

void read_completions(int fd) {
    char buffer[256];
    struct iovec iov = { .iov_base = buffer, .iov_len = sizeof(buffer) };
    struct msghdr msg = {0};
    char control[100];
    
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    while (recvmsg(fd, &msg, MSG_ERRQUEUE | MSG_DONTWAIT) > 0);
}

void *send_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    long *bytes_sent = malloc(sizeof(long));
    *bytes_sent = 0;
    int sock = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return bytes_sent;
    }

    // Critical: Enable Zero Copy per socket
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_ZEROCOPY, &opt, sizeof(opt))) {
        perror("Zero Copy not supported");
        close(sock);
        return bytes_sent;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(args->port);
    inet_pton(AF_INET, args->server_ip, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        return bytes_sent;
    }

    // Zero Copy typically requires page-aligned buffers for max efficiency
    size_t total_size = NUM_FIELDS * STRING_LEN;
    char *flat_buffer = malloc(total_size);
    generate_random_string(flat_buffer, total_size);

    time_t end_time = time(NULL) + DURATION_SECONDS;

    while (time(NULL) < end_time) {
        ssize_t sent = send(sock, flat_buffer, total_size, MSG_ZEROCOPY);
        
        if (sent < 0) {
            if (errno == ENOBUFS) {
                read_completions(sock);
            }
        } else {
            *bytes_sent += sent;
            if (*bytes_sent % (total_size * 10) == 0) read_completions(sock);
        }
    }
    
    read_completions(sock);
    close(sock);
    free(flat_buffer);
    return bytes_sent;
}

int main(int argc, char *argv[]) {
    char *server_ip = "127.0.0.1";
    int port = SERVER_PORT;
    int num_threads = 1;

    if (argc > 1) server_ip = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) num_threads = atoi(argv[3]);

    printf("Starting 0-Copy Client: %d threads -> %s:%d\n", num_threads, server_ip, port);

    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];

    for (int i = 0; i < num_threads; i++) {
        args[i].server_ip = server_ip;
        args[i].port = port;
        pthread_create(&threads[i], NULL, send_thread, &args[i]);
    }

    long total_bytes = 0;
    for (int i = 0; i < num_threads; i++) {
        void *ret_val;
        pthread_join(threads[i], &ret_val);
        total_bytes += *(long*)ret_val;
        free(ret_val);
    }

    printf("0-Copy Total: %ld bytes sent across %d threads.\n", total_bytes, num_threads);
    return 0;
}