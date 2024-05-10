#ifndef SISCI_PERF_TIMER_CONTROLLED_VARIABLE_H
#define SISCI_PERF_TIMER_CONTROLLED_VARIABLE_H

void init_timer(time_t seconds);
volatile sig_atomic_t *get_timer_expired();
void start_timer();
void destroy_timer();

#endif //SISCI_PERF_TIMER_CONTROLLED_VARIABLE_H
