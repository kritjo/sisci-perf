#ifndef SISCI_PERF_DMA_VEC_H
#define SISCI_PERF_DMA_VEC_H

#define MEASURE_SECONDS 2
#define MIN_DMA_TRANSFER_SIZE 1024
#define MIN_DMA_VEC_EL_SIZE MIN_DMA_TRANSFER_SIZE

void run_experiment_dma_vec(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt);

#endif //SISCI_PERF_DMA_VEC_H