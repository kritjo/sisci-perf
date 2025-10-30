#include "common.h"

/* Generic benchmark op: do one unit of work. `i` is the iteration index. */
typedef void (*bench_op_fn)(int i, void *ctx);

/* Generic timer/throughput benchmark around a user-supplied op */
static void run_benchmark(bench_op_fn op, void *ctx, int size)
{
    timer_start_t timer_start;
    StartTimer(&timer_start);

    for (int i = 0; i < ILOOPS; i++) {
        op(i, ctx);
    }

    double totalTimeUs         = StopTimer(timer_start);
    double totalBytes          = (double)size * ILOOPS;
    double averageTransferTime = totalTimeUs / (double)ILOOPS;
    double MB_pr_second        = totalBytes / totalTimeUs;

    printf("%7llu            %6.2f us              %7.2f MBytes/s\n",
           (unsigned long long)size, averageTransferTime, MB_pr_second);
}

/* --- Example op implementation: SCIMemCpy --- */
typedef struct {
    sci_sequence_t remote_sequence;
    void          *local_address;
    sci_map_t      remote_map;
    int            size;
    volatile void *remote_address;
} memcpy_ctx_t;

static void memcpy_op(int i, void *vctx)
{
    (void)i; /* iteration not used for this op */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
    SEOE(SCIMemCpy, ctx->remote_sequence, ctx->local_address, ctx->remote_map,
         NO_OFFSET, ctx->size, NO_FLAGS);
}

static void memcpy_two_halves_op(int i, void *vctx)
{
    (void)i; /* iteration not used */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    int half = ctx->size / 2;
    if (half <= 0) {
        /* Nothing meaningful to do if size < 2. You could fallback to one copy. */
        return;
    }

    char *local = (char *)ctx->local_address;

    /* First half: [0 .. half-1] -> remote offset 0 */
    SEOE(SCIMemCpy, ctx->remote_sequence, local,
         ctx->remote_map, NO_OFFSET, half, NO_FLAGS);

    /* Second half: [half .. 2*half-1] -> remote offset `half` */
    SEOE(SCIMemCpy, ctx->remote_sequence, local + half,
         ctx->remote_map, half, half, NO_FLAGS);
}

static void memcpy_64_chunks_op(int i, void *vctx)
{
    (void)i; /* iteration index unused */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    const int CHUNK = 64;
    int remaining = ctx->size;
    int offset = 0;

    uint64_t *local = (uint64_t *)ctx->local_address;
    volatile uint64_t *remote = (volatile uint64_t *)ctx->remote_address;

    while (remaining > 0) {
        *remote = *local;
        remaining -= CHUNK;
        remote++;
        local++;
    }
}

static void memcpy_nonvol_64_chunks_op(int i, void *vctx)
{
    (void)i; /* iteration index unused */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    const int CHUNK = 64;
    int remaining = ctx->size;
    int offset = 0;

    uint64_t *local = (uint64_t *)ctx->local_address;
    uint64_t *remote = (uint64_t *)ctx->remote_address;

    while (remaining > 0) {
        *remote = *local;
        remaining -= CHUNK;
        remote++;
        local++;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <remote node id> <size>\n", argv[0]);
        exit(1);
    }

    int remote_node_id = atoi(argv[1]);
    int size = atoi(argv[2]);

    printf("Remote node id: %d\n", remote_node_id);
    printf("Size: %d\n", size);

    sci_desc_t sd;
    sci_error_t error;
    void* local_address;
    int i;
    char *malloc_x = malloc(size + 63);
    while ((size_t)malloc_x & 0x3f) {
        malloc_x++;
    }
    local_address = (void *) malloc_x;
    for (i=0; i<size; ++i) malloc_x[i] = i & 255;

    sci_remote_segment_t remote_segment;
    sci_map_t remote_map;
    volatile void* remote_address;
    sci_sequence_t remote_sequence;

    SEOE(SCIInitialize, NO_FLAGS);
    SEOE(SCIOpen, &sd, NO_FLAGS);

    SEOE(SCIConnectSegment, sd, &remote_segment, remote_node_id, SEGMENT_ID,
         ADAPTER_NO, NO_CALLBACK, NO_ARGS, SCI_INFINITE_TIMEOUT, NO_FLAGS);
    remote_address = (volatile void*)
        SCIMapRemoteSegment(remote_segment, &remote_map, NO_OFFSET,
                            size, NO_ADDRESS_HINT, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) return 1;
    SEOE(SCICreateMapSequence, remote_map, &remote_sequence, NO_FLAGS);

    /* Prepare context for our op */
    memcpy_ctx_t ctx = {
        .remote_sequence = remote_sequence,
        .local_address   = local_address,
        .remote_map      = remote_map,
        .size            = size,
        .remote_address  = remote_address
    };

    /* Warm-up using the same op */
    for (int i = 0; i < WULOOPS; i++) {
        memcpy_op(i, &ctx);
    }
    printf("Warmed up!\n");

    for (int csize = 1; csize <= size; csize *= 2) {
        printf("Size: %d\n", csize);

        /* Timed benchmark with op callback */
        run_benchmark(memcpy_op, &ctx, csize);
        
        printf("Benchmarking split it in two. Should be same speed:\n");
        run_benchmark(memcpy_two_halves_op, &ctx, csize);

        if (csize >= 64) {
            printf("Benchmarking memcpy 64 byte chunks:\n");
            run_benchmark(memcpy_64_chunks_op, &ctx, csize);
        
            printf("Benchmarking memcpy 64 byte chunks nonvolatile:\n");
            run_benchmark(memcpy_nonvol_64_chunks_op, &ctx, csize);
        }
    }
    SEOE(SCIRemoveSequence, remote_sequence, NO_FLAGS);
    SEOE(SCIUnmapSegment, remote_map, NO_FLAGS);
    SEOE(SCIDisconnectSegment, remote_segment, NO_FLAGS);
    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
    return 0;
}

