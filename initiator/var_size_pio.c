#include <sisci_api.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "var_size_pio.h"
#include "sisci_glob_defs.h"
#include "protocol.h"
#include "timer_controlled_variable.h"


static volatile sig_atomic_t *timer_expired;
static unsigned long long operations = 0;

static void write_pio_byte(volatile void *uncasted_data) {
    volatile unsigned char *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE; i++) {
            data[i] = 0x01;
            operations++;
        }
    }
}

static void read_pio_byte(volatile void *uncasted_data, pid_t main_pid) {
    volatile unsigned char *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE; i++) {
            if (data[i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %d\n", i, data[i]);
                kill(main_pid, SIGTERM);
            }
            operations++;
        }
    }
}

static void write_pio_word(volatile void *uncasted_data) {
    volatile unsigned short *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE / 2; i++) {
            data[i] = 0x01;
            operations += 2;
        }
    }
}

static void read_pio_word(volatile void *uncasted_data, pid_t main_pid) {
    volatile unsigned short *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE / 2; i++) {
            if (data[i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %d\n", i, data[i]);
                kill(main_pid, SIGTERM);
            }
            operations += 2;
        }
    }
}

static void write_pio_dword(volatile void *uncasted_data) {
    volatile unsigned int *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE / 4; i++) {
            data[i] = 0x01;
            operations += 4;
        }
    }
}

static void read_pio_dword(volatile void *uncasted_data, pid_t main_pid) {
    volatile unsigned int *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE / 4; i++) {
            if (data[i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %d\n", i, data[i]);
                kill(main_pid, SIGTERM);
            }
            operations += 4;
        }
    }
}

static void write_pio_qword(volatile void *uncasted_data) {
    volatile unsigned long *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE / 8; i++) {
            data[i] = 0x01;
            operations += 8;
        }
    }
}

static void read_pio_qword(volatile void *uncasted_data, pid_t main_pid) {
    volatile unsigned long *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE / 8; i++) {
            if (data[i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %lu\n", i, data[i]);
                kill(main_pid, SIGTERM);
            }
            operations += 8;
        }
    }
}

static void write_pio_dqword(volatile void *uncasted_data) {
    volatile unsigned long long *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE / 16; i++) {
            data[i] = 0x01;
            operations += 16;
        }
    }
}

static void read_pio_dqword(volatile void *uncasted_data, pid_t main_pid) {
    volatile unsigned long long *data = uncasted_data;
    while (!*timer_expired) {
        for (uint32_t i = 0; i < MAX_SEGMENT_SIZE / 16; i++) {
            if (data[i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %llu\n", i, data[i]);
                kill(main_pid, SIGTERM);
            }
            operations += 16;
        }
    }
}


static void memcpy_write_pio(void *source, sci_sequence_t sequence, sci_map_t remote_map, size_t size) {
    while (!*timer_expired) {
        SEOE(SCIMemCpy, sequence, source, remote_map, 0, size, NO_FLAGS);
        operations += size;
    }
}

static void memcpy_read_pio(void *dest, sci_sequence_t sequence, sci_map_t remote_map, size_t size) {
    while (!*timer_expired) {
        SEOE(SCIMemCpy, sequence, dest, remote_map, 0, size, SCI_FLAG_BLOCK_READ);
        operations += size;
    }
}

void run_var_size_experiment_pio(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    // Order one segment from one peer
    sci_remote_segment_t segment;
    sci_map_t map;
    order_t order;
    delivery_t delivery;
    unsigned int size;
    volatile void *data;
    sci_error_t error;
    sci_sequence_t sequence;
    size_t transfer_size;
    void *local_data;
    void (*write_pio_func)(volatile void *) = NULL;
    void (*read_pio_func)(volatile void *, pid_t) = NULL;

    local_data = malloc(MAX_SEGMENT_SIZE);
    if (local_data == NULL) {
        fprintf(stderr, "Failed to allocate memory for local data\n");
        kill(main_pid, SIGTERM);
    }

    init_timer(MEASURE_SECONDS);
    timer_expired = get_timer_expired();

    order.commandType = COMMAND_TYPE_CREATE;
    order.orderType = ORDER_TYPE_SEGMENT;
    order.size = MAX_SEGMENT_SIZE;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }

    SEOE(SCIConnectSegment, sd, &segment, delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    data = SCIMapRemoteSegment(segment, &map, NO_OFFSET, MAX_SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Failed to map segment: %d\n", error);
        kill(main_pid, SIGTERM);
    }

    transfer_size = 1;
    while (transfer_size <= 16) {
        switch (transfer_size) {
            case 1:
                write_pio_func = write_pio_byte;
                read_pio_func = read_pio_byte;
                break;
            case 2:
                write_pio_func = write_pio_word;
                read_pio_func = read_pio_word;
                break;
            case 4:
                write_pio_func = write_pio_dword;
                read_pio_func = read_pio_dword;
                break;
            case 8:
                write_pio_func = write_pio_qword;
                read_pio_func = read_pio_qword;
                break;
            case 16:
                write_pio_func = write_pio_dqword;
                read_pio_func = read_pio_dqword;
                break;
            default:
                fprintf(stderr, "Invalid transfer size %zu\n", transfer_size);
                kill(main_pid, SIGTERM);
        }

        printf("Starting PIO write %zu bytes for %d seconds\n", transfer_size, MEASURE_SECONDS);
        operations = 0;
        start_timer();
        write_pio_func(data);
        printf("    operations: %llu\n", operations);

        printf("Starting PIO read %zu bytes for %d seconds\n", transfer_size, MEASURE_SECONDS);
        operations = 0;
        start_timer();
        read_pio_func(data, main_pid);
        printf("    operations: %llu\n", operations);

        transfer_size *= 2;
    }

    SEOE(SCICreateMapSequence, map, &sequence, NO_FLAGS);
    SEOE(SCIStartSequence, sequence, NO_FLAGS);

    transfer_size = 4;
    while (transfer_size <= MAX_SEGMENT_SIZE) {
        printf("Starting MemCpy write %zu bytes for %d seconds\n", transfer_size, MEASURE_SECONDS);
        operations = 0;
        start_timer();
        memcpy_write_pio(local_data, sequence, map, transfer_size);
        printf("    operations: %llu\n", operations);

        printf("Starting MemCpy read %zu bytes for %d seconds\n", transfer_size, MEASURE_SECONDS);
        operations = 0;
        start_timer();
        memcpy_read_pio(local_data, sequence, map, transfer_size);
        printf("    operations: %llu\n", operations);

        transfer_size *= 2;
    }

    free(local_data);

    destroy_timer();

    SEOE(SCIRemoveSequence, sequence, NO_FLAGS);

    SEOE(SCIUnmapSegment, map, NO_FLAGS);

    SEOE(SCIDisconnectSegment, segment, NO_FLAGS);

    order.commandType = COMMAND_TYPE_DESTROY;
    order.id = delivery.id;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }
}
