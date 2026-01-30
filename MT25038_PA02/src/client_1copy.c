/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Write a C TCP client using sendmsg and iovec to avoid application-layer buffer copying."
 */

#include "common.h"
#include <pthread.h>

#define DURATION_SECONDS 10
#define STRING_LEN 4096 

typedef struct {
    char *server_ip;
    int port;
} ThreadArgs;

void *send_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    long *bytes_sent = malloc(sizeof(long));
    *bytes_sent = 0;
    int sock = 0;

    // 1. Create Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
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

    // 2. Prepare Data (One-Copy Specific)
    Message msg;
    struct iovec iov[NUM_FIELDS];
    
    // Each thread needs its own data to send
    for (int i = 0; i < NUM_FIELDS; i++) {
        msg.lengths[i] = STRING_LEN;
        msg.fields[i] = malloc(STRING_LEN);
        generate_random_string(msg.fields[i], STRING_LEN);
        
        // Point iovec directly to the string (Scatter-Gather)
        iov[i].iov_base = msg.fields[i];
        iov[i].iov_len = STRING_LEN;
    }

    // 3. Construct the Message Header
    struct msghdr message_header = {0};
    message_header.msg_iov = iov;
    message_header.msg_iovlen = NUM_FIELDS;

    time_t end_time = time(NULL) + DURATION_SECONDS;

    // 4. Send Loop
    while (time(NULL) < end_time) {
        // One-Copy: Kernel reads directly from iov pointers
        ssize_t sent = sendmsg(sock, &message_header, 0);
        if (sent > 0) *bytes_sent += sent;
    }

    close(sock);
    for (int i = 0; i < NUM_FIELDS; i++) free(msg.fields[i]);
    return bytes_sent;
}

int main(int argc, char *argv[]) {
    char *server_ip = "127.0.0.1";
    int port = SERVER_PORT;
    int num_threads = 1;

    if (argc > 1) server_ip = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) num_threads = atoi(argv[3]);

    printf("Starting 1-Copy Client: %d threads -> %s:%d\n", num_threads, server_ip, port);

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

    printf("1-Copy Total: %ld bytes sent across %d threads.\n", total_bytes, num_threads);
    return 0;
}