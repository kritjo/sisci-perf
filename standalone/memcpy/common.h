#include "sisci_api.h"
#include "sisci_types.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define ADAPTER_NO 0
#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARGS 0
#define NO_OFFSET 0
#define NO_ADDRESS_HINT 0
#define SEGMENT_ID 1
#define ILOOPS 50000
#define WULOOPS 50000
#define FALSE 0
#define TRUE 1
#define MIB 1048576
#define MICRO_SECONDS 1000000

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

typedef struct {
    unsigned long long start_cputick;
    unsigned int       start_walltick;
    struct timeval     start_time;
} timer_start_t;

void StartTimer(timer_start_t *timer_start);
double StopTimer(timer_start_t timer_start);
