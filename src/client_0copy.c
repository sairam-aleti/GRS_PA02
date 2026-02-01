/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Write a C TCP client using MSG_ZEROCOPY and process the ERRQUEUE to confirm completion."
 */

#include "common.h"
#include <pthread.h>

#define DURATION_SECONDS 10

typedef struct {
    char *server_ip;
    int port;
    int msg_size;
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
    long *stats = malloc(2 * sizeof(long));
    stats[0] = 0; stats[1] = 0;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_ZEROCOPY, &opt, sizeof(opt))) {
        perror("Zero Copy Setup Failed");
        close(sock); return stats;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(args->port);
    inet_pton(AF_INET, args->server_ip, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock); return stats;
    }

    size_t total_size = args->msg_size;
    if (total_size == 0) total_size = 4096;
    
    // Allocate buffer
    char *flat_buffer = malloc(total_size);
    generate_random_string(flat_buffer, total_size);

    time_t end_time = time(NULL) + DURATION_SECONDS;
    struct timespec start, end;
    long operations = 0;

    while (time(NULL) < end_time) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        ssize_t sent = send(sock, flat_buffer, total_size, MSG_ZEROCOPY);
        
        clock_gettime(CLOCK_MONOTONIC, &end);

        if (sent < 0) {
            if (errno == ENOBUFS) read_completions(sock);
        } else {
            stats[0] += sent;
            long us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
            stats[1] += us;
            operations++;
            
            // Periodically clean the error queue to prevent stall
            if (stats[0] % (total_size * 5) == 0) read_completions(sock);
        }
    }
    
    read_completions(sock);
    if (operations > 0) stats[1] = stats[1] / operations;

    close(sock);
    free(flat_buffer);
    return stats;
}

int main(int argc, char *argv[]) {
    char *server_ip = "127.0.0.1";
    int port = SERVER_PORT;
    int num_threads = 1;
    int msg_size = 4096;

    if (argc > 1) server_ip = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) num_threads = atoi(argv[3]);
    if (argc > 4) msg_size = atoi(argv[4]);

    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];

    for (int i = 0; i < num_threads; i++) {
        args[i].server_ip = server_ip;
        args[i].port = port;
        args[i].msg_size = msg_size;
        pthread_create(&threads[i], NULL, send_thread, &args[i]);
    }

    long total_bytes = 0;
    long total_latency = 0;
    for (int i = 0; i < num_threads; i++) {
        void *ret_val;
        pthread_join(threads[i], &ret_val);
        long *ret = (long*)ret_val;
        total_bytes += ret[0];
        total_latency += ret[1];
        free(ret_val);
    }

    printf("Stats: %ld %ld\n", total_bytes, total_latency / num_threads);
    return 0;
}