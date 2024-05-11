#ifndef SISCI_PERF_COMMON_READ_WRITE_FUNCTIONS_H
#define SISCI_PERF_COMMON_READ_WRITE_FUNCTIONS_H

#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "sisci_api.h"
#include "sisci_glob_defs.h"

extern volatile sig_atomic_t timer_expired;

static unsigned long long operations;

void init_timer(time_t seconds);
volatile sig_atomic_t *get_timer_expired();
void start_timer();
void destroy_timer();

typedef void (*pio_function_t)(
        volatile void *uncasted_data[],
        size_t segment_size,
        uint32_t num_segments);

static inline void write_pio_byte(volatile void *uncasted_data[],
                                  size_t segment_size,
                                  uint32_t num_segments) {
    volatile unsigned char **data = (volatile unsigned char **) uncasted_data;

    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size; i++) {
            data[i % num_segments][i] = 0x01;
            operations++;
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
            operations++;
        }
    }
}

static inline void write_pio_word(volatile void *uncasted_data[],
                                  size_t segment_size,
                                  uint32_t num_segments) {
    volatile unsigned short **data = (volatile unsigned short **) uncasted_data;
    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 2; i++) {
            data[i % num_segments][i] = 0x01;
            operations += 2;
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
            operations += 2;
        }
    }
}

static inline void write_pio_dword(volatile void *uncasted_data[],
                                   size_t segment_size,
                                   uint32_t num_segments) {
    volatile unsigned int **data = (volatile unsigned int **) uncasted_data;
    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 4; i++) {
            data[i % num_segments][i] = 0x01;
            operations += 4;
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
            operations += 4;
        }
    }
}

static inline void write_pio_qword(volatile void *uncasted_data[],
                                   size_t segment_size,
                                   uint32_t num_segments) {
    volatile unsigned long **data = (volatile unsigned long **) uncasted_data;
    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 8; i++) {
            data[i % num_segments][i] = 0x01;
            operations += 8;
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
            operations += 8;
        }
    }
}

static inline void write_pio_dqword(volatile void *uncasted_data[],
                                    size_t segment_size,
                                    uint32_t num_segments) {
    volatile unsigned long long **data = (volatile unsigned long long **) uncasted_data;
    operations = 0;
    while (!timer_expired) {
        for (uint32_t i = 0; i < segment_size / 16; i++) {
            data[i % num_segments][i] = 0x01;
            operations += 16;
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
            operations += 16;
        }
    }
}


static inline void memcpy_write_pio(void *source,
                                    sci_sequence_t sequence,
                                    sci_map_t remote_map,
                                    size_t size) {
    operations = 0;
    while (!timer_expired) {
        SEOE(SCIMemCpy, sequence, source, remote_map, 0, size, NO_FLAGS);
        operations += size;
    }
}

static inline void memcpy_read_pio(void *dest,
                                   sci_sequence_t sequence,
                                   sci_map_t remote_map, size_t size) {
    operations = 0;
    while (!timer_expired) {
        SEOE(SCIMemCpy, sequence, dest, remote_map, 0, size, SCI_FLAG_BLOCK_READ);
        operations += size;
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
        operations += transfer_size;
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
        operations += transfer_size;
    }
}


#endif //SISCI_PERF_COMMON_READ_WRITE_FUNCTIONS_H
