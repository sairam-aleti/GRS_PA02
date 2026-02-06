/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Create a C header file for a network benchmark defining a struct with 8 dynamically allocated string fields."
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define SERVER_PORT 8080
#define NUM_FIELDS 8  // Required by Assignment

// Unified Message Structure
// Satisfies "Structure comprising 8 dynamically allocated string fields"
typedef struct {
    char *fields[NUM_FIELDS]; 
    size_t lengths[NUM_FIELDS];
} Message;

// Function Prototypes
void setup_server_socket(int *sockfd);
void setup_client_socket(int *sockfd, const char *server_ip);
void generate_random_string(char *str, int length);

// Helper to create the complex message
static inline Message* create_complex_message(size_t size_per_field) {
    Message *msg = (Message*)malloc(sizeof(Message));
    if (!msg) return NULL;

    for (int i = 0; i < NUM_FIELDS; i++) {
        msg->lengths[i] = size_per_field;
        msg->fields[i] = (char*)malloc(size_per_field);
        if (msg->fields[i]) {
            // Fill with dummy data (A, B, C...)
            memset(msg->fields[i], 'A' + i, size_per_field);
            msg->fields[i][size_per_field - 1] = '\0';
        }
    }
    return msg;
}

// Helper to free the complex message
static inline void free_complex_message(Message *msg) {
    if (!msg) return;
    for (int i = 0; i < NUM_FIELDS; i++) {
        free(msg->fields[i]);
    }
    free(msg);
}

#endif