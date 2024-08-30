#ifndef SISCI_PERF_INITIATOR_MAIN_H
#define SISCI_PERF_INITIATOR_MAIN_H

#define READABLE_PRINT 0
#define MACHINE_PRINT 1
#define readable_printf(...) if(READABLE_PRINT == 1) printf(__VA_ARGS__)
#define machine_printf(...) if(MACHINE_PRINT == 1) printf(__VA_ARGS__)


#define MAX_PEERS 4

#define SISCI_PERF_PIO 0
#define SISCI_PERF_DMA 0
#define SISCI_PERF_SCALE_UP 0
#define SISCI_PERF_SCALE_OUT 1
#define SISCI_PERF_VAR_SIZE 0
#define SISCI_PERF_VAR_SIZE_DMA 0
#define SISCI_PERF_DMA_VEC 0
#define SISCI_PERF_INT 0
#define SISCI_PERF_DATA_INT 0
#define SISCI_PERF_PING_PONG_PIO 0
#define SISCI_PERF_BROADCAST_PIO 1

// Broadcast + Global DMA causes kernel panic :)
#define UNSAFE_DO_NOT_USE_WILL_CAUSE_KERNEL_PANIC_SISCI_PERF_BROADCAST_DMA 0

#endif //SISCI_PERF_INITIATOR_MAIN_H
