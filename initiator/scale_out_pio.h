#ifndef SISCI_PERF_SCALE_OUT_PIO_H
#define SISCI_PERF_SCALE_OUT_PIO_H

#include "sisci_types.h"
#include <sys/types.h>

#define MEASURE_SECONDS 2

void run_scale_out_segment_experiment_pio(sci_desc_t sd, pid_t main_pid, uint32_t num_peers, sci_remote_data_interrupt_t *order_interrupts, sci_local_data_interrupt_t delivery_interrupt);

#endif //SISCI_PERF_SCALE_OUT_PIO_H
