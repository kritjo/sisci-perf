#ifndef KRITJOLIB_SISCI_RMA
#define KRITJOLIB_SISCI_RMA

#include "sisci_types.h"
#include "sisci_arg_types.h"

void rma_init(segment_remote_args_t *remote);
void rma_sequence_start(sci_sequence_t sequence);
void rma_sequence_init(sci_map_t map, sci_sequence_t *sequence);
void rma_flush(sci_sequence_t sequence);
void rma_check(sci_sequence_t sequence);
void rma_destroy_sequence(sci_sequence_t sequence);
void rma_destroy(sci_map_t remote_map);

#endif //KRITJOLIB_SISCI_RMA
