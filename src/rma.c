#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "rma.h"

#include "rdma_buff.h"
#include "lib_rma.h"

void rma(sci_remote_segment_t remote_segment, bool check, bool send) {
    printf("RMA\n");
    volatile rdma_buff_t *buff;
    sci_sequence_t sequence;
    rdma_buff_t rdma_buff;

    segment_remote_args_t remote = {0};
    remote.segment = remote_segment;

    printf("Initializing remote segment at %p\n", remote_segment);
    rma_init(&remote);

    if (check) {
        printf("Initializing sequence\n");
        rma_sequence_init(remote.map, &sequence);
    }
    buff = (rdma_buff_t *) remote.address;

    if (send) {
        rdma_buff.done = 0;
        strcpy(rdma_buff.word, "OK");

        *buff = rdma_buff;
        buff->done = 1;
        printf("Wrote to remote segment\n");
    } else {
        printf("Waiting for transfer\n");
        while (!buff->done);
        printf("RDMA Done! Word: %s\n", buff->word);
    }

    if (check) {
        rma_flush(sequence);
        rma_check(sequence);
        rma_destroy_sequence(sequence);
    }

    rma_destroy(remote.map);
}