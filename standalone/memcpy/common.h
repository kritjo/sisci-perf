#include "sisci_api.h"
#include "sisci_types.h"

#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define ADAPTER_NO 0
#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARGS 0
#define NO_OFFSET 0
#define NO_ADDRESS_HINT 0
#define SEGMENT_ID 1
#define SEGMENT_SIZE 2097152
#define ILOOPS 50000
#define FALSE 0
#define TRUE 1

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
    unsigned long long start_cputick;       /* for rtdsc() */
    unsigned int       start_walltick;      /* for GetTickCount() and tickGet() */
    struct timeval     start_time;          /* for gettimeofday() */
} timer_start_t;

typedef unsigned long long elog_time;
static uint64_t elog_clock_scale = 0;
static int elog_initialized = FALSE;
unsigned long long rdtsc_inst();
int elog_processor_speed_calibrate(void);
int init_elog();
void rdtsc(elog_time* tp /* MODIFIED */);
void StartTimer(timer_start_t *timer_start);
double StopTimer(timer_start_t timer_start);
double elog_timediff_usecs(elog_time* start_time, elog_time* end_time);
