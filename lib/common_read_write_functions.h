#ifndef SISCI_PERF_COMMON_READ_WRITE_FUNCTIONS_H
#define SISCI_PERF_COMMON_READ_WRITE_FUNCTIONS_H

#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sisci_api.h"
#include "sisci_glob_defs.h"
#include "protocol.h"

// Set to 1 to disable the checks for DMA completeness.
// This is useful when we want to measure how fast we can post DMA transfers to the queue.
// This will however potentially lead to crashes if the DMA transfers are not completed before the next one is posted.
#define SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS 0

// Set to 1 to allow the SCI_FLAG_DMA_WAIT.
// According to kritjo, this flag does not yield any performance improvements and leads to crashes. Therefore it is
// disabled by default.
// Do not enable this flag unless you know what you are doing.
#define SISCI_PERF_ALLOW_DMA_VEC_WAIT 0

#define NO_SEQUENCE NULL
typedef enum {
    PIO_FLAG_NO_SEQ,
    PIO_FLAG_FLUSH,
    PIO_FLAG_CHK_SEQ
} pio_flags_t;

extern volatile sig_atomic_t timer_expired;

static unsigned long long operations;

void init_timer(time_t seconds);
void start_timer();
void destroy_timer();

typedef void (*pio_write_function_t)(
        volatile void *uncasted_data[],
        size_t segment_size,
        uint32_t num_segments,
        sci_sequence_t sequence,
        pio_flags_t flags);

typedef void (*pio_read_function_t)(
        volatile void *uncasted_data[],
        size_t segment_size,
        uint32_t num_segments);

static inline void write_pio_byte(volatile void *uncasted_data[],
                                  size_t segment_size,
                                  uint32_t num_segments,
                                  sci_sequence_t sequence,
                                  pio_flags_t flags) {
    volatile unsigned char **data = (volatile unsigned char **) uncasted_data;
    sci_sequence_status_t status;
    sci_error_t error;

    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size; i++) {
            data[i % num_segments][i] = 0x01;
            if (flags == PIO_FLAG_FLUSH) SCIFlush(sequence, NO_FLAGS);
            else if (flags == PIO_FLAG_CHK_SEQ) {
                status = SCICheckSequence(sequence, NO_FLAGS, &error);
                if (error != SCI_ERR_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence failed with error %s\n", SCIGetErrorString(error));
                    exit(EXIT_FAILURE);
                }
                if (status != SCI_SEQ_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence returned status %d\n", status);
                    exit(EXIT_FAILURE);
                }
            }
            if (!timer_expired) operations++;
        }
    }
}

static inline void read_pio_byte(volatile void *uncasted_data[],
                                 size_t segment_size,
                                 uint32_t num_segments) {
    volatile unsigned char **data = (volatile unsigned char **) uncasted_data;

    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size; i++) {
            if (data[i % num_segments][i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %d\n", i, data[i % num_segments][i]);
                exit(EXIT_FAILURE);
            }
            if (!timer_expired) operations++;
        }
    }
}

static inline void write_pio_word(volatile void *uncasted_data[],
                                  size_t segment_size,
                                  uint32_t num_segments,
                                  sci_sequence_t sequence,
                                  pio_flags_t flags) {
    volatile unsigned short **data = (volatile unsigned short **) uncasted_data;
    sci_sequence_status_t status;
    sci_error_t error;

    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 2; i++) {
            data[i % num_segments][i] = 0x01;
            if (flags == PIO_FLAG_FLUSH) SCIFlush(sequence, NO_FLAGS);
            else if (flags == PIO_FLAG_CHK_SEQ) {
                status = SCICheckSequence(sequence, NO_FLAGS, &error);
                if (error != SCI_ERR_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence failed with error %s\n", SCIGetErrorString(error));
                    exit(EXIT_FAILURE);
                }
                if (status != SCI_SEQ_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence returned status %d\n", status);
                    exit(EXIT_FAILURE);
                }
            }
            if (!timer_expired) operations++;
        }
    }
}

static inline void read_pio_word(volatile void *uncasted_data[],
                                 size_t segment_size,
                                 uint32_t num_segments) {
    volatile unsigned short **data = (volatile unsigned short **) uncasted_data;

    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 2; i++) {
            if (data[i % num_segments][i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %d\n", i, data[i % num_segments][i]);
                exit(EXIT_FAILURE);
            }
            if (!timer_expired) operations++;
        }
    }
}

static inline void write_pio_dword(volatile void *uncasted_data[],
                                   size_t segment_size,
                                   uint32_t num_segments,
                                   sci_sequence_t sequence,
                                   pio_flags_t flags) {
    volatile unsigned int **data = (volatile unsigned int **) uncasted_data;
    sci_sequence_status_t status;
    sci_error_t error;

    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 4; i++) {
            data[i % num_segments][i] = 0x01;
            if (flags == PIO_FLAG_FLUSH) SCIFlush(sequence, NO_FLAGS);
            else if (flags == PIO_FLAG_CHK_SEQ) {
                status = SCICheckSequence(sequence, NO_FLAGS, &error);
                if (error != SCI_ERR_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence failed with error %s\n", SCIGetErrorString(error));
                    exit(EXIT_FAILURE);
                }
                if (status != SCI_SEQ_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence returned status %d\n", status);
                    exit(EXIT_FAILURE);
                }
            }
            if (!timer_expired) operations++;
        }
    }
}

static inline void read_pio_dword(volatile void *uncasted_data[],
                                  size_t segment_size,
                                  uint32_t num_segments) {
    volatile unsigned int **data = (volatile unsigned int **) uncasted_data;

    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 4; i++) {
            if (data[i % num_segments][i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %d\n", i, data[i % num_segments][i]);
                exit(EXIT_FAILURE);
            }
            if (!timer_expired) operations++;
        }
    }
}

static inline void write_pio_qword(volatile void *uncasted_data[],
                                   size_t segment_size,
                                   uint32_t num_segments,
                                   sci_sequence_t sequence,
                                   pio_flags_t flags) {
    volatile unsigned long **data = (volatile unsigned long **) uncasted_data;
    sci_sequence_status_t status;
    sci_error_t error;

    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 8; i++) {
            data[i % num_segments][i] = 0x01;
            if (flags == PIO_FLAG_FLUSH) SCIFlush(sequence, NO_FLAGS);
            else if (flags == PIO_FLAG_CHK_SEQ) {
                status = SCICheckSequence(sequence, NO_FLAGS, &error);
                if (error != SCI_ERR_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence failed with error %s\n", SCIGetErrorString(error));
                    exit(EXIT_FAILURE);
                }
                if (status != SCI_SEQ_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence returned status %d\n", status);
                    exit(EXIT_FAILURE);
                }
            }
            if (!timer_expired) operations++;
        }
    }
}

static inline void read_pio_qword(volatile void *uncasted_data[],
                                  size_t segment_size,
                                  uint32_t num_segments) {
    volatile unsigned long **data = (volatile unsigned long **) uncasted_data;
    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 8; i++) {
            if (data[i % num_segments][i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %lu\n", i, data[i % num_segments][i]);
                exit(EXIT_FAILURE);
            }
            if (!timer_expired) operations++;
        }
    }
}

static inline void write_pio_dqword(volatile void *uncasted_data[],
                                    size_t segment_size,
                                    uint32_t num_segments,
                                    sci_sequence_t sequence,
                                    pio_flags_t flags) {
    volatile unsigned long long **data = (volatile unsigned long long **) uncasted_data;
    sci_sequence_status_t status;
    sci_error_t error;

    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 16; i++) {
            data[i % num_segments][i] = 0x01;
            if (flags == PIO_FLAG_FLUSH) SCIFlush(sequence, NO_FLAGS);
            else if (flags == PIO_FLAG_CHK_SEQ) {
                status = SCICheckSequence(sequence, NO_FLAGS, &error);
                if (error != SCI_ERR_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence failed with error %s\n", SCIGetErrorString(error));
                    exit(EXIT_FAILURE);
                }
                if (status != SCI_SEQ_OK) {
                    fprintf(stderr, "ERROR: SCICheckSequence returned status %d\n", status);
                    exit(EXIT_FAILURE);
                }
            }
            if (!timer_expired) operations++;
        }
    }
}

static inline void read_pio_dqword(volatile void *uncasted_data[],
                                   size_t segment_size,
                                   uint32_t num_segments) {
    volatile unsigned long long **data = (volatile unsigned long long **) uncasted_data;
    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 16; i++) {
            if (data[i % num_segments][i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %llu\n", i, data[i % num_segments][i]);
                exit(EXIT_FAILURE);
            }
            if (!timer_expired) operations++;
        }
    }
}


static inline void memcpy_write_pio(void *source,
                                    sci_sequence_t sequence[],
                                    sci_map_t remote_map[],
                                    size_t size,
                                    uint32_t num_segments) {
    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < size; i++) {
            SEOE(SCIMemCpy, sequence[i%num_segments], source, remote_map[i%num_segments], NO_OFFSET, size, NO_FLAGS);
            if (!timer_expired) operations++;
        }
    }
}

static inline void memcpy_read_pio(void *dest,
                                   sci_sequence_t sequence,
                                   sci_map_t remote_map, size_t size) {
    operations = 0;
    while (!timer_expired) {
        SEOE(SCIMemCpy, sequence, dest, remote_map, NO_OFFSET, size, SCI_FLAG_BLOCK_READ);
        if (!timer_expired) operations++;
    }
}

/**
 * @note As per the documentation "An application is allowed to start another transfer on a queue only after the
 * previous transfer for that queue has completed". Therefore we have to check the state of the queue before starting.
 */

static inline void block_for_dma(sci_dma_queue_t dma_queue) {
    sci_dma_queue_state_t dma_state;
    dma_state = SCIDMAQueueState(dma_queue);

    while (dma_state == SCI_DMAQUEUE_POSTED) {
        SEOE(SCIWaitForDMAQueue, dma_queue, SCI_INFINITE_TIMEOUT, NO_FLAGS);
        dma_state = SCIDMAQueueState(dma_queue);
    }

    switch (dma_state) {
        case SCI_DMAQUEUE_IDLE:
        case SCI_DMAQUEUE_DONE:
            break;
        case SCI_DMAQUEUE_GATHER:
            fprintf(stderr, "ERROR: DMA queue is in gather state, don't know what to do\n");
            exit(EXIT_FAILURE);
            break;
        case SCI_DMAQUEUE_POSTED:
            // Case to satisfy clang-tidy - and make it explicit that this should not happen
            fprintf(stderr, "ERROR: Logical error, this should not happen\n");
            exit(EXIT_FAILURE);
            break;
        case SCI_DMAQUEUE_ABORTED:
            fprintf(stderr, "ERROR: DMA queue is in aborted state\n");
            exit(EXIT_FAILURE);
            break;
        case SCI_DMAQUEUE_ERROR:
            fprintf(stderr, "ERROR: DMA queue is in error state\n");
            exit(EXIT_FAILURE);
            break;
    }
}

static inline void write_dma(sci_local_segment_t local_segment,
                             sci_remote_segment_t remote_segment,
                             sci_dma_queue_t dma_queue,
                             size_t transfer_size) {
    operations = 0;

    while (!timer_expired) {
        SEOE(SCIStartDmaTransfer, dma_queue, local_segment, remote_segment, 0, transfer_size, 0, NO_CALLBACK, NO_ARG,
             SCI_FLAG_DMA_GLOBAL);
#if SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS == 0
        block_for_dma(dma_queue);
#endif // SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS
        if (!timer_expired) operations++;
    }
}

static inline void write_dma_broadcast(sci_local_segment_t local_segment,
                             sci_remote_segment_t remote_segment,
                             sci_dma_queue_t dma_queue,
                             size_t transfer_size) {
    operations = 0;

    while (!timer_expired) {
        SEOE(SCIStartDmaTransfer, dma_queue, local_segment, remote_segment, 0, transfer_size, 0, NO_CALLBACK, NO_ARG,
             SCI_FLAG_DMA_GLOBAL | SCI_FLAG_BROADCAST);
#if SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS == 0
        block_for_dma(dma_queue);
#endif // SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS
        if (!timer_expired) operations++;
    }
}

static inline void read_dma(sci_local_segment_t local_segment,
                            sci_remote_segment_t remote_segment,
                            sci_dma_queue_t dma_queue,
                            size_t transfer_size) {
    operations = 0;

    while (!timer_expired) {
        SEOE(SCIStartDmaTransfer, dma_queue, local_segment, remote_segment, 0, transfer_size, 0, NO_CALLBACK, NO_ARG,
             SCI_FLAG_DMA_GLOBAL | SCI_FLAG_DMA_READ);
#if SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS == 0
        block_for_dma(dma_queue);
#endif // SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS
        if (!timer_expired) operations++;
    }
}

static inline void write_dma_vec(sci_local_segment_t local_segment,
                                 sci_remote_segment_t remote_segment,
                                 sci_dma_queue_t dma_queue,
                                 dis_dma_vec_t *dma_vec,
                                 size_t vec_len,
                                 bool block) {
    unsigned int flags = block ? SCI_FLAG_DMA_GLOBAL | SCI_FLAG_DMA_WAIT : SCI_FLAG_DMA_GLOBAL;

#if SISCI_PERF_ALLOW_DMA_VEC_WAIT == 0
    if (block) {
        fprintf(stderr, "ERROR: Blocking DMA transfers with SCI_FLAG_DMA_WAIT is not allowed\n");
        exit(EXIT_FAILURE);
    }
#endif // SISCI_PERF_ALLOW_DMA_VEC_WAIT

    operations = 0;

    while (!timer_expired) {
        SEOE(SCIStartDmaTransferVec, dma_queue, local_segment, remote_segment, vec_len, dma_vec, NO_CALLBACK, NO_ARG, flags);
#if SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS == 0
        if (!block) block_for_dma(dma_queue);
#endif // SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS
        if (!timer_expired) operations++;
    }
}

static inline void read_dma_vec(sci_local_segment_t local_segment,
                                sci_remote_segment_t remote_segment,
                                sci_dma_queue_t dma_queue,
                                dis_dma_vec_t *dma_vec,
                                size_t vec_len,
                                bool block) {
    unsigned int flags = block ? SCI_FLAG_DMA_GLOBAL | SCI_FLAG_DMA_READ | SCI_FLAG_DMA_WAIT : SCI_FLAG_DMA_GLOBAL | SCI_FLAG_DMA_READ;

#if SISCI_PERF_ALLOW_DMA_VEC_WAIT == 0
    if (block) {
        fprintf(stderr, "ERROR: Blocking DMA transfers with SCI_FLAG_DMA_WAIT is not allowed\n");
        exit(EXIT_FAILURE);
    }
#endif // SISCI_PERF_ALLOW_DMA_VEC_WAIT

    operations = 0;
    sci_error_t error;

    while (!timer_expired) {
        SCIStartDmaTransferVec(dma_queue, local_segment, remote_segment, vec_len, dma_vec, NO_CALLBACK, NO_ARG, flags, &error);
#if SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS == 0
        if (!block) block_for_dma(dma_queue);
#endif // SISCI_PERF_DISABLE_DMA_COMPLETENESS_CHECKS
        if (!timer_expired) operations++;
    }
}

static inline void trigger_interrupt(sci_remote_interrupt_t remote_interrupt) {
    operations = 0;
    while (!timer_expired) {
        SEOE(SCITriggerInterrupt, remote_interrupt, NO_FLAGS);
        if (!timer_expired) operations++;
    }
}

static inline void trigger_data_interrupt(sci_remote_data_interrupt_t remote_interrupt, void *data, unsigned int length) {
    sci_error_t error;
    bool nospc_error = false;
    operations = 0;

    while (!timer_expired) {
        SCITriggerDataInterrupt(remote_interrupt, data, length, NO_FLAGS, &error);
        if (error != SCI_ERR_OK && error != SCI_ERR_NOSPC) {
            fprintf(stderr, "ERROR: SCITriggerDataInterrupt failed with error code %d\n", error);
            exit(EXIT_FAILURE);
        } else {
            if (!timer_expired) operations++;
        }
        if (error == SCI_ERR_NOSPC) {
            nospc_error = true;
        }
    }

    if (nospc_error) {
        fprintf(stderr, "ERROR: SCITriggerDataInterrupt failed with error code SCI_ERR_NOSPC one or more times in a masked context\n");
    }
}

static inline void ping_pong_pio(const unsigned char *local_ptr, peer_ping_pong_segment_t *remote_ptr, sci_sequence_t sequence, sci_map_t local_map) {
    operations = 0;
    unsigned char curr_counter = 0;

    while (!timer_expired) {
        curr_counter++;

        remote_ptr->counter = curr_counter;
        SCIFlush(sequence, NO_FLAGS);

        while (*local_ptr != curr_counter) {
            SCIFlush(sequence, NO_FLAGS);
        }

        if (!timer_expired) operations++;
    }
}

#endif //SISCI_PERF_COMMON_READ_WRITE_FUNCTIONS_H
