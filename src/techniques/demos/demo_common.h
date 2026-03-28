#ifndef HEAPLENS_TECHNIQUES_DEMO_COMMON_H
#define HEAPLENS_TECHNIQUES_DEMO_COMMON_H

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline void heaplens_demo_stage(const char *message) {
    if (message) {
        write(3, message, strlen(message));
        write(3, "\n", 1);
    }

    /* SIGSTOP gives the ptrace parent a deterministic point to refresh every panel. */
    raise(SIGSTOP);
}

static inline void *heaplens_demo_xmalloc(size_t size) {
    void *pointer = malloc(size);
    if (!pointer) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    return pointer;
}

static inline char *heaplens_demo_make_chunk(size_t size, unsigned char fill) {
    char *buffer = heaplens_demo_xmalloc(size);
    memset(buffer, fill, size);
    return buffer;
}

#define HEAPLENS_DEFINE_PLACEHOLDER_DEMO(TITLE)                  \
    int main(void) {                                             \
        char *a = heaplens_demo_make_chunk(0x40, 'A');           \
        char *b = heaplens_demo_make_chunk(0x40, 'B');           \
        heaplens_demo_stage(TITLE " : allocated");               \
        free(a);                                                 \
        heaplens_demo_stage(TITLE " : freed-a");                 \
        a = heaplens_demo_make_chunk(0x40, 'C');                 \
        heaplens_demo_stage(TITLE " : reallocated");             \
        free(a);                                                 \
        free(b);                                                 \
        heaplens_demo_stage(TITLE " : done");                    \
        return 0;                                                \
    }

#endif
