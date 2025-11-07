#include "common.h"
#include "string.h"
#include <immintrin.h>
#include <stdint.h>

/* Generic benchmark op: do one unit of work. `i` is the iteration index. */
typedef void (*bench_op_fn)(int i, void *ctx, int size);
typedef void (*bench_cb_fn)(void *ctx);

/* Generic timer/throughput benchmark around a user-supplied op */
static void run_benchmark(bench_op_fn op, void *ctx, int size, char *thing, bench_cb_fn cb)
{
    timer_start_t timer_start;
    StartTimer(&timer_start);

    for (int i = 0; i < ILOOPS; i++) {
        op(i, ctx, size);
    }

    if (cb) {
        cb(ctx);
    }

    double totalTimeUs         = StopTimer(timer_start);
    double totalBytes          = (double)size * ILOOPS;
    double averageTransferTime = totalTimeUs / (double)ILOOPS;
    double MB_pr_second = totalBytes/totalTimeUs;

    printf("%8llu|%6.2f us|%10.2f MB/s|%s\n", 
           (unsigned long long)size, averageTransferTime, MB_pr_second, thing);
}

/* --- Example op implementation: SCIMemCpy --- */
typedef struct {
    sci_sequence_t remote_sequence;
    void          *local_address;
    sci_map_t      remote_map;
    int            size;
    volatile void *remote_address;
} memcpy_ctx_t;

static void memcopy_op(int i, void *vctx, int size)
{
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
    memcpy(ctx->remote_address, ctx->local_address, size);
}

static void scicopy_op(int i, void *vctx, int size)
{
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
    SEOE(SCIMemCpy, ctx->remote_sequence, ctx->local_address,
         ctx->remote_map, NO_OFFSET, size, NO_FLAGS);
}

static void scicopy_two_halves_op(int i, void *vctx, int size)
{
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
    int half = size / 2;
    int tail = size - half;            // keep odd byte(s)

    char *local = (char *)ctx->local_address;

    if (half > 0) {
        SEOE(SCIMemCpy, ctx->remote_sequence, local,
             ctx->remote_map, NO_OFFSET, half, NO_FLAGS);
        SEOE(SCIMemCpy, ctx->remote_sequence, local + half,
             ctx->remote_map, half, tail, NO_FLAGS);
    }
}

static void memcpy_32_chunks_op(int i, void *vctx, int size)
{
    (void)i; /* iteration index unused */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    int remaining = size;
    int offset = 0;

    uint32_t *local = (uint32_t *)ctx->local_address;
    volatile uint32_t *remote = (volatile uint32_t *)ctx->remote_address;

    while (remaining > 0) {
        *remote = *local;
        remaining -= 4;
        remote++;
        local++;
    }
}

static void memcpy_64_chunks_op(int i, void *vctx, int size)
{
    (void)i; /* iteration index unused */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    int remaining = size;
    int offset = 0;

    uint64_t *local = (uint64_t *)ctx->local_address;
    volatile uint64_t *remote = (volatile uint64_t *)ctx->remote_address;

    while (remaining > 0) {
        *remote = *local;
        remaining -= 8;
        remote++;
        local++;
    }
}

static void memcpy_nonvol_64_chunks_op(int i, void *vctx, int size)
{
    (void)i; /* iteration index unused */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    int remaining = size;
    int offset = 0;

    uint64_t *local = (uint64_t *)ctx->local_address;
    uint64_t *remote = (uint64_t *)ctx->remote_address;

    while (remaining > 0) {
        *remote = *local;
        remaining -= 8;
        remote++;
        local++;
    }
}

/* AVX2 non-temporal copy to remote mapped memory.
   - Aligns dest to 64B to avoid split lines
   - Uses _mm256_stream_si256 (32B NT stores) in a 64B unrolled loop
   - Light source prefetch
   - _mm_sfence at the end to complete NT stores
*/
static void memcpy_avx2_nt_op(int i, void *vctx, int size)
{
    (void)i;
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

#if defined(__AVX2__)
    const uint8_t *src = (const uint8_t *)ctx->local_address;
    /* Cast away volatile for intrinsics; we still do a final sfence. */
    uint8_t *dst = (uint8_t *)(uintptr_t)ctx->remote_address;

    int n = size;

    /* For smaller blocks, temporal stores are usually faster. */
    if (n < 256) {
        /* Tiny fallback (cached scalar) */
        while (n >= 8) { *(uint64_t*)dst = *(const uint64_t*)src; dst+=8; src+=8; n-=8; }
        while (n--) { *dst++ = *src++; }
        _mm_sfence();
        return;
    }

    /* Align dest to 64B (cache line). */
    uintptr_t mis = (uintptr_t)dst & 63u;
    if (mis) {
        int lead = 64 - (int)mis;
        if (lead > n) lead = n;
        int l = lead;
        while (l >= 8) { *(uint64_t*)dst = *(const uint64_t*)src; dst+=8; src+=8; l-=8; }
        while (l--) { *dst++ = *src++; }
        n -= lead;
    }

    /* Main loop: 64B per iter (2x 32B streaming stores). */
    while (n >= 64) {
        _mm_prefetch((const char*)src + 512, _MM_HINT_T0);
        __m256i a = _mm256_loadu_si256((const __m256i*)(src));
        __m256i b = _mm256_loadu_si256((const __m256i*)(src + 32));
        _mm256_stream_si256((__m256i*)(dst),       a);
        _mm256_stream_si256((__m256i*)(dst + 32),  b);
        src += 64; dst += 64; n -= 64;
    }

    if (n >= 32) {
        __m256i v = _mm256_loadu_si256((const __m256i*)src);
        _mm256_stream_si256((__m256i*)dst, v);
        src += 32; dst += 32; n -= 32;
    }
    if (n >= 16) {
        __m128i v16 = _mm_loadu_si128((const __m128i*)src);
        _mm_stream_si128((__m128i*)dst, v16);  /* 16B NT store (requires 16B alignment; we’re 64B aligned here) */
        src += 16; dst += 16; n -= 16;
    }
    while (n >= 8) { *(uint64_t*)dst = *(const uint64_t*)src; dst+=8; src+=8; n-=8; }
    while (n--)     { *dst++ = *src++; }

    /* Ensure NT stores are globally visible before returning. */
    _mm_sfence();
#else
    /* Build without AVX2? Fall back to your 64-bit scalar op. */
    printf("ERROR: NO AVX2 support\n");
#endif
}

/* AVX2 cached stores; lets caches do the work (good for <=~256B–1KB, or if NT isn’t ideal). */
static void memcpy_avx2_cached_op(int i, void *vctx, int size)
{
    (void)i;
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
#if defined(__AVX2__)
    const uint8_t *src = (const uint8_t *)ctx->local_address;
    uint8_t *dst = (uint8_t *)(uintptr_t)ctx->remote_address;

    int n = size;
    while (n >= 64) {
        __m256i a = _mm256_loadu_si256((const __m256i*)src);
        __m256i b = _mm256_loadu_si256((const __m256i*)(src + 32));
        _mm256_storeu_si256((__m256i*)dst,      a);
        _mm256_storeu_si256((__m256i*)(dst+32), b);
        src += 64; dst += 64; n -= 64;
    }
    if (n >= 32) {
        __m256i v = _mm256_loadu_si256((const __m256i*)src);
        _mm256_storeu_si256((__m256i*)dst, v);
        src += 32; dst += 32; n -= 32;
    }
    if (n >= 16) {
        __m128i v16 = _mm_loadu_si128((const __m128i*)src);
        _mm_storeu_si128((__m128i*)dst, v16);
        src += 16; dst += 16; n -= 16;
    }
    while (n >= 8) { *(uint64_t*)dst = *(const uint64_t*)src; dst+=8; src+=8; n-=8; }
    while (n--)     { *dst++ = *src++; }
#else
    printf("ERROR: NO AVX2 support\n");
#endif
}

static void avx2_cached_fence_cb(void *ctx)
{
    (void)ctx;
#if defined(__AVX2__)
    _mm_mfence();
#endif
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
        scicopy_op(i, &ctx, size);
    }
    printf("Warmed up!\n");

    for (int csize = 1; csize <= size; csize *= 2) {
        /* Timed benchmark with op callback */
        run_benchmark(scicopy_op, &ctx, csize, "scicopy_op", NULL);
        
        run_benchmark(scicopy_two_halves_op, &ctx, csize, "scicopy_two_halves_op", NULL);

        run_benchmark(memcopy_op, &ctx, csize, "memcopy_op", NULL);

        run_benchmark(memcpy_avx2_nt_op, &ctx, csize, "memcpy_avx2_nt_op", NULL);

        run_benchmark(memcpy_avx2_cached_op, &ctx, csize, "memcpy_avx2_cached_op", avx2_cached_fence_cb);

        if (csize >= 32) {
            run_benchmark(memcpy_32_chunks_op, &ctx, csize, "memcpy_32_chunks_op", NULL);
        }

        if (csize >= 64) {
            run_benchmark(memcpy_64_chunks_op, &ctx, csize, "memcpy_64_chunks_op", NULL);
        
            run_benchmark(memcpy_nonvol_64_chunks_op, &ctx, csize, "memcpy_nonvol_64_chunks_op", NULL);
        }
    }
    SEOE(SCIRemoveSequence, remote_sequence, NO_FLAGS);
    SEOE(SCIUnmapSegment, remote_map, NO_FLAGS);
    SEOE(SCIDisconnectSegment, remote_segment, NO_FLAGS);
    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
    return 0;
}

