/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Write a C TCP client that serializes 8 strings into a single buffer and sends it."
 */

#include "common.h"
#include <pthread.h>

#define DURATION_SECONDS 10
#define STRING_LEN 4096 

// Structure to pass arguments to each thread
typedef struct {
    char *server_ip;
    int port;
    int thread_id;
} ThreadArgs;

// This function runs inside each thread
void *send_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int sock = 0;
    struct sockaddr_in serv_addr;
    long *bytes_sent = malloc(sizeof(long)); // Return value
    *bytes_sent = 0;

    // 1. Create Socket (Each thread gets its own socket)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return bytes_sent;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(args->port);
    inet_pton(AF_INET, args->server_ip, &serv_addr.sin_addr);

    // 2. Connect
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        return bytes_sent;
    }

    // 3. Prepare Data (Local to thread to avoid race conditions)
    Message msg;
    size_t total_payload_size = 0;
    for (int i = 0; i < NUM_FIELDS; i++) {
        msg.lengths[i] = STRING_LEN;
        msg.fields[i] = malloc(STRING_LEN);
        generate_random_string(msg.fields[i], STRING_LEN);
        total_payload_size += STRING_LEN;
    }

    char *flat_buffer = malloc(total_payload_size);
    time_t end_time = time(NULL) + DURATION_SECONDS;

    // 4. Send Loop
    while (time(NULL) < end_time) {
        size_t offset = 0;
        // Copy 1: Serialize
        for (int i = 0; i < NUM_FIELDS; i++) {
            memcpy(flat_buffer + offset, msg.fields[i], msg.lengths[i]);
            offset += msg.lengths[i];
        }
        // Copy 2: Send
        ssize_t sent = send(sock, flat_buffer, total_payload_size, 0);
        if (sent > 0) *bytes_sent += sent;
    }

    // Cleanup
    close(sock);
    free(flat_buffer);
    for (int i = 0; i < NUM_FIELDS; i++) free(msg.fields[i]);
    
    return bytes_sent;
}

int main(int argc, char *argv[]) {
    char *server_ip = "127.0.0.1";
    int port = SERVER_PORT;
    int num_threads = 1; // Default

    // Arguments: ./client_2copy <IP> <PORT> <THREADS>
    if (argc > 1) server_ip = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) num_threads = atoi(argv[3]);

    printf("Starting 2-Copy Client: %d threads -> %s:%d\n", num_threads, server_ip, port);

    pthread_t threads[num_threads];
    ThreadArgs args[num_threads];

    // Spawn Threads
    for (int i = 0; i < num_threads; i++) {
        args[i].server_ip = server_ip;
        args[i].port = port;
        args[i].thread_id = i;
        pthread_create(&threads[i], NULL, send_thread, &args[i]);
    }

    // Join Threads and Collect Stats
    long total_bytes = 0;
    for (int i = 0; i < num_threads; i++) {
        void *ret_val;
        pthread_join(threads[i], &ret_val);
        total_bytes += *(long*)ret_val;
        free(ret_val);
    }

    printf("2-Copy Total: %ld bytes sent across %d threads.\n", total_bytes, num_threads);
    return 0;
}