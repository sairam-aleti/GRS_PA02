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
#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/errqueue.h>
#include <sys/uio.h>

#define SERVER_PORT 8080
#define NUM_FIELDS 8

typedef struct {
    char *fields[NUM_FIELDS];
    size_t lengths[NUM_FIELDS];
} Message;

// Utils
void generate_random_string(char *buffer, size_t length);
long get_time_diff_microseconds(struct timespec start, struct timespec end);

#endif