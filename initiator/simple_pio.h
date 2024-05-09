#ifndef SISCI_PERF_SIMPLE_PIO_H
#define SISCI_PERF_SIMPLE_PIO_H

#define MEASURE_SECONDS 2

void run_single_segment_experiment_pio(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt);

#endif //SISCI_PERF_SIMPLE_PIO_H
