#ifndef SISCI_PERF_SIMPLE_DATA_INTERRUPT_H
#define SISCI_PERF_SIMPLE_DATA_INTERRUPT_H

#define MEASURE_SECONDS 2
#define DATA_INT_DATA_LENGTH 50

void run_experiment_data_interrupt(sci_desc_t sd, __attribute__((unused)) pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt);

#endif //SISCI_PERF_SIMPLE_DATA_INTERRUPT_H
