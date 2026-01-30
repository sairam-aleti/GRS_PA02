/*
 * AI DECLARATION
 * Tool Used: Gemini 2.0
 * Prompt: "Implement C helper functions to generate random alphanumeric strings and calculate time differences."
 */

#include "common.h"

void generate_random_string(char *buffer, size_t length) {
    if (length == 0) return;
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < length - 1; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[length - 1] = '\0';
}

long get_time_diff_microseconds(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000L + 
           (end.tv_nsec - start.tv_nsec) / 1000;
}