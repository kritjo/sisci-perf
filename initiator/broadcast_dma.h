#ifndef SISCI_PERF_BROADCAST_DMA_H
#define SISCI_PERF_BROADCAST_DMA_H

#include <sisci_types.h>
#include <sys/types.h>
#include <stdint.h>

void run_broadcast_dma_experiment(sci_desc_t sd, pid_t main_pid, uint32_t num_peers, sci_remote_data_interrupt_t *order_interrupts, sci_local_data_interrupt_t delivery_interrupt);

#endif //SISCI_PERF_BROADCAST_DMA_H
