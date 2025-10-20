#include "common.h"
#define MICRO_SECONDS 1000000

void StartTimer(timer_start_t *timer_start)
{
    if (gettimeofday(&timer_start->start_time,(struct timezone *)0) != 0) {
        fprintf(stderr, "gettimeofday() error; timings suspect.\n");
    }
    return;
}

double StopTimer(timer_start_t timer_start)
{
    struct          timeval  end_time;
    long            wall_ticks;
    elog_time       end_cputick;  

    /* This is the default for all os except OS_IS_WINDOWS and OS_IS_QNX */
    if (gettimeofday(&end_time, (struct timezone *)NULL) != 0) {
        fprintf(stderr, "gettimeofday() error; timings suspect.\n");
    }
    wall_ticks = ((double)(((end_time.tv_sec - timer_start.start_time.tv_sec) * MICRO_SECONDS) + 
                           (end_time.tv_usec - timer_start.start_time.tv_usec)));

    return(wall_ticks);
}