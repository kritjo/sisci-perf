#ifndef SISCI_PERF_SISCI_GLOB_DEFS_H
#define SISCI_PERF_SISCI_GLOB_DEFS_H

#define SEGMENT_SIZE 4096
#define MAX_SEGMENT_SIZE 16777216
#define MAX_BROADCAST_SEGMENT_SIZE 2097152
#define BROADCAST_GROUP_ID 1
#define DMA_QUEUE_MAX_ENTRIES 4

#define ADAPTER_NO 0

#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARG 0
#define NO_OFFSET 0
#define NO_SUGGESTED_ADDRESS 0

#include <unistd.h>
#include <limits.h>
#include <stdio.h>

// SISCI Exit On Error -- SEOE
#define SEOE(func, ...) \
do {                    \
    char hostname[HOST_NAME_MAX + 1]; \
    gethostname(hostname, HOST_NAME_MAX + 1); \
    sci_error_t seoe_error; \
    func(__VA_ARGS__, &seoe_error); \
    if (seoe_error != SCI_ERR_OK) { \
        fprintf(stderr, "[%s] %s failed: %s\n", hostname, #func, SCIGetErrorString(seoe_error)); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#endif //SISCI_PERF_SISCI_GLOB_DEFS_H
