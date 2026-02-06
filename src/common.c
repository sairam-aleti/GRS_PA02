/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Implement C helper functions to generate random alphanumeric strings and calculate time differences."
 */

#include "common.h"
#include <netinet/in.h>
#include <sys/socket.h>

// Helper to generate random data for the fields
void generate_random_string(char *str, int length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (length <= 0) return;
    for (int i = 0; i < length - 1; i++) {
        int key = rand() % (int)(sizeof(charset) - 1);
        str[i] = charset[key];
    }
    str[length - 1] = '\0';
}

void setup_server_socket(int *sockfd) {
    struct sockaddr_in address;
    int opt = 1;

    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Reuse address/port to prevent bind errors
    if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    if (bind(*sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(*sockfd, 16) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

void setup_client_socket(int *sockfd, const char *server_ip) {
    struct sockaddr_in serv_addr;

    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(*sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }
}