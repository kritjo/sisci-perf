#ifndef SISCI_PERF_PING_PONG_THREAD_H
#define SISCI_PERF_PING_PONG_THREAD_H

typedef struct {
    sci_map_t map;

    sci_remote_segment_t remote_segment;
    sci_map_t remote_map;
} ping_pong_cleanup_arg_t;

typedef struct {
    sci_desc_t sd;
    sci_local_segment_t segment;
    unsigned int initiator_node_id;
} ping_pong_thread_arg_t;

_Noreturn void *ping_pong_thread_start(void *arg);

#endif //SISCI_PERF_PING_PONG_THREAD_H
