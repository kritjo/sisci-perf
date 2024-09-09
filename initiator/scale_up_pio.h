#ifndef SISCI_PERF_SCALE_UP_PIO_H
#define SISCI_PERF_SCALE_UP_PIO_H

#include "sisci_types.h"
#include <sys/types.h>

#define MEASURE_SECONDS 2
#define MAX_SEGMENTS 8

/*
 * The num_segments define how far we should scale up, to a maximum of MAX_SEGMENTS. The order interrupt
 * indices correspond to which node we should run the experiment on. E.g. if we want the experiment for one, two and
 * three segments to run on node n001, but the rest on x004, we can put the same interrupts in the first three and last
 * three indices for the interrupt array.
 */
void run_scale_up_segment_experiment_pio(sci_desc_t sd, pid_t main_pid, uint32_t num_segments, uint32_t scale_up_node_ids[], sci_remote_data_interrupt_t order_interrupt[], sci_local_data_interrupt_t delivery_interrupt);

#endif //SISCI_PERF_SCALE_UP_PIO_H
