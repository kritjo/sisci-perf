#include <sisci_api.h>
#include <sisci_error.h>
#include <sisci_types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// SISCI Exit On Error -- SEOE
#define SEOE(func, ...) \
do {                    \
    sci_error_t seoe_error; \
    func(__VA_ARGS__, &seoe_error); \
    if (seoe_error != SCI_ERR_OK) { \
        printf("Error in function %s: %s\n", #func, SCIGetErrorString(seoe_error)); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define NO_FLAGS 0
#define NO_CALLBACK NULL
#define NO_ARGS NULL
#define NO_OFFSET 0
#define NO_ADDRESS_HINT 0
#define SEGMENT_SIZE 16777216
#define SERVER_SEGMENT_ID 42
#define CLIENT_SEGMENT_ID 43
#define ADAPTER_NO 0

#define WRITEBACK_SIZE 64

typedef struct {
    int counter;
    char writeback_buffer[WRITEBACK_SIZE];
} ping_pong_segment_t;