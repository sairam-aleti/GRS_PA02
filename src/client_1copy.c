/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Write TCP client (sendmsg version) to accept message size argument and measure latency."
 */
#include "common.h"
#include <pthread.h>

#define DURATION_SECONDS 10

typedef struct {
    char *server_ip;
    int port;
    int msg_size;
} ThreadArgs;

void *send_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    long *stats = malloc(2 * sizeof(long));
    stats[0] = 0; stats[1] = 0;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(args->port);
    inet_pton(AF_INET, args->server_ip, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock); return stats;
    }

    Message msg;
    struct iovec iov[NUM_FIELDS];
    
    // Distribute the requested msg_size across the 8 fields
    int field_len = args->msg_size / NUM_FIELDS;
    if (field_len == 0) field_len = 1;

    for (int i = 0; i < NUM_FIELDS; i++) {
        msg.lengths[i] = field_len;
        msg.fields[i] = malloc(field_len);
        generate_random_string(msg.fields[i], field_len);
        iov[i].iov_base = msg.fields[i];
        iov[i].iov_len = field_len;
    }

    struct msghdr message_header = {0};
    message_header.msg_iov = iov;
    message_header.msg_iovlen = NUM_FIELDS;

    time_t end_time = time(NULL) + DURATION_SECONDS;
    struct timespec start, end;
    long operations = 0;

    while (time(NULL) < end_time) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // 1-Copy Send
        ssize_t sent = sendmsg(sock, &message_header, 0);
        
        clock_gettime(CLOCK_MONOTONIC, &end);

        if (sent > 0) {
            stats[0] += sent;
            long us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
            stats[1] += us;
            operations++;
        }
    }
    if (operations > 0) stats[1] = stats[1] / operations;

    close(sock);
    for (int i = 0; i < NUM_FIELDS; i++) free(msg.fields[i]);
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