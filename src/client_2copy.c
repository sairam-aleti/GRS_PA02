/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Write TCP client to accept message size as argument and measure latency per send."
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
    // We use an array to pass back two values: [0]=bytes_sent, [1]=total_latency_us
    long *stats = malloc(2 * sizeof(long)); 
    stats[0] = 0; stats[1] = 0;
    
    int sock = 0;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return stats;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(args->port);
    inet_pton(AF_INET, args->server_ip, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock); return stats;
    }

    // Prepare Data
    Message msg;
    size_t total_payload_size = 0;
    int field_len = args->msg_size / NUM_FIELDS; // Distribute size across fields
    if (field_len == 0) field_len = 1;

    // Allocate memory for this thread
    for (int i = 0; i < NUM_FIELDS; i++) {
        msg.lengths[i] = field_len;
        msg.fields[i] = malloc(field_len);
        generate_random_string(msg.fields[i], field_len);
        total_payload_size += field_len;
    }

    char *flat_buffer = malloc(total_payload_size);
    time_t end_time = time(NULL) + DURATION_SECONDS;
    long operations = 0;

    struct timespec start, end;

    while (time(NULL) < end_time) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        // Serialize
        size_t offset = 0;
        for (int i = 0; i < NUM_FIELDS; i++) {
            memcpy(flat_buffer + offset, msg.fields[i], msg.lengths[i]);
            offset += msg.lengths[i];
        }
        // Send
        ssize_t sent = send(sock, flat_buffer, total_payload_size, 0);
        
        clock_gettime(CLOCK_MONOTONIC, &end);

        if (sent > 0) {
            stats[0] += sent;
            // Calculate latency in microseconds
            long us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
            stats[1] += us;
            operations++;
        }
    }

    // Store Average Latency
    if (operations > 0) stats[1] = stats[1] / operations;

    close(sock);
    free(flat_buffer);
    for (int i = 0; i < NUM_FIELDS; i++) free(msg.fields[i]);
    return stats;
}

int main(int argc, char *argv[]) {
    char *server_ip = "127.0.0.1";
    int port = SERVER_PORT;
    int num_threads = 1;
    int msg_size = 4096; // Default size

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

    // Output Format: "Stats: BYTES_SENT AVG_LATENCY_US"
    printf("Stats: %ld %ld\n", total_bytes, total_latency / num_threads);
    return 0;
}