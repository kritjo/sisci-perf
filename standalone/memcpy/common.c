#include "common.h"

unsigned long long rdtsc_inst()
{
    union { 
        struct {
            uint32_t low;
            uint32_t high;
        } _x;
        uint64_t value;
    } v;

    asm volatile ("rdtsc");
    asm volatile ("mov %%eax, %0" : "=m" (v._x.low));
    asm volatile ("mov %%edx, %0" : "=m" (v._x.high));

    return v.value;

}

int elog_processor_speed_calibrate(void)
{
        struct timeval tv1, tv2;
        elog_time ts1, ts2;
        double gtod1, gtod2, hrc1, hrc2;

        rdtsc(&ts1);
        gettimeofday(&tv1, NULL);
        rdtsc(&ts2);
        gettimeofday(&tv2, NULL);

        gtod1 = tv1.tv_sec * 1e6 + tv1.tv_usec;
        gtod2 = tv2.tv_sec * 1e6 + tv2.tv_usec;
        hrc1 = ts1;
        hrc2 = ts2;

        elog_clock_scale = ((hrc2 - hrc1) *1e6/ (double) (gtod2 - gtod1));
        return (elog_clock_scale != 0) ? TRUE : FALSE;
}

int init_elog()
{
    if ( elog_initialized ) {
        if ( elog_clock_scale == 0 ) {
            return FALSE;
        } else {
            return TRUE;
        }
    }

    elog_initialized = TRUE;

    return elog_processor_speed_calibrate();
}

void rdtsc(elog_time* tp /* MODIFIED */)
{
    *tp = rdtsc_inst();
}

void StartTimer(timer_start_t *timer_start)
{
   rdtsc(&timer_start->start_cputick);	/* CPU clock time */
   return;
}

double elog_timediff_usecs(elog_time* start_time, elog_time* end_time)
{
    return (double)
        (((*end_time - *start_time)*1000000) / elog_clock_scale);
}

double StopTimer(timer_start_t timer_start)
{
    struct          timeval  end_time;
    long            wall_ticks;
    elog_time       end_cputick;  

    if (init_elog()) {
        
        /* 
         * Use Read Time-Stamp Counter provided by X86 architecture
         */
        rdtsc(&end_cputick);		/* CPU process time */
        return(elog_timediff_usecs(&timer_start.start_cputick, &end_cputick));
    }
}