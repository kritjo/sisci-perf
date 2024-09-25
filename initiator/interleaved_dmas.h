#ifndef SISCI_PERF_INTERLEAVED_DMAS_H
#define SISCI_PERF_INTERLEAVED_DMAS_H

#include <sisci_types.h>
#include <sys/types.h>
#include <stdint.h>

#define MEASURE_SECONDS 2
#define NUM_CONCURRENT_SEGMENTS 2
#define TRANSFER_SIZE 4096

void run_single_segment_experiment_interleaved_dma(sci_desc_t sd, pid_t main_pid, uint32_t num_peers, sci_remote_data_interrupt_t *order_interrupts, sci_local_data_interrupt_t delivery_interrupt);

#endif //SISCI_PERF_INTERLEAVED_DMAS_H
