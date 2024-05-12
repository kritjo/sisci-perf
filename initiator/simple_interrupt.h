#ifndef SISCI_PERF_SIMPLE_INTERRUPT_H
#define SISCI_PERF_SIMPLE_INTERRUPT_H

#define MEASURE_SECONDS 2

void run_experiment_interrupt(sci_desc_t sd, __attribute__((unused)) pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt);

#endif //SISCI_PERF_SIMPLE_INTERRUPT_H
