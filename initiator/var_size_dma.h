#ifndef SISCI_PERF_VAR_SIZE_DMA_H
#define SISCI_PERF_VAR_SIZE_DMA_H

#include "sisci_types.h"
#include <sys/types.h>

#define MEASURE_SECONDS 2
#define MIN_MEASURE_DMA_TRANSFER_SIZE 1024
#define MAX_MEASURE_DMA_TRANSFER_SIZE MAX_SEGMENT_SIZE

void run_var_size_experiment_dma(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt);

#endif //SISCI_PERF_VAR_SIZE_DMA_H
