#include "common.h"
#include "string.h"
#include <immintrin.h>
#include <stdint.h>

/* Generic benchmark op: do one unit of work. `i` is the iteration index. */
typedef void (*bench_op_fn)(int i, void *ctx, int bytes);
typedef void (*bench_cb_fn)(void *ctx);

/* Generic timer/throughput benchmark around a user-supplied op */
static void run_benchmark(bench_op_fn op, void *ctx, int bytes, char *thing, bench_cb_fn cb)
{
    timer_start_t timer_start;
    StartTimer(&timer_start);

    for (int i = 0; i < ILOOPS; i++) {
        op(i, ctx, bytes);
    }

    if (cb) {
        cb(ctx);
    }

    double totalTimeUs         = StopTimer(timer_start);
    double totalBytes          = (double)bytes * ILOOPS;
    double averageTransferTime = totalTimeUs / (double)ILOOPS;
    double MB_pr_second = totalBytes/totalTimeUs;

    printf("%8llu|%6.2f us|%10.2f MB/s|%s\n", 
           (unsigned long long)bytes, averageTransferTime, MB_pr_second, thing);
}

/* --- Example op implementation: SCIMemCpy --- */
typedef struct {
    sci_sequence_t remote_sequence;
    void          *local_address;
    sci_map_t      remote_map;
    int            bytes;
    volatile void *remote_address;
    uint32_t       trans_size;
} memcpy_ctx_t;

static void memcopy_op(int i, void *vctx, int bytes)
{
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
    memcpy(ctx->remote_address, ctx->local_address, bytes);
}

static void scicopy_op(int i, void *vctx, int bytes)
{
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
    SEOE(SCIMemCpy, ctx->remote_sequence, ctx->local_address,
         ctx->remote_map, NO_OFFSET, bytes, NO_FLAGS);
}

static void scicopy_two_halves_op(int i, void *vctx, int bytes)
{
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
    int half = bytes / 2;
    int tail = bytes - half;            // keep odd byte(s)

    char *local = (char *)ctx->local_address;

    if (half > 0) {
        SEOE(SCIMemCpy, ctx->remote_sequence, local,
             ctx->remote_map, NO_OFFSET, half, NO_FLAGS);
        SEOE(SCIMemCpy, ctx->remote_sequence, local + half,
             ctx->remote_map, half, tail, NO_FLAGS);
    }
}

static inline void memcpy_8_chunks_op(int i, void *vctx, int bytes)
{
    (void)i;
    memcpy_ctx_t *ctx = vctx;

    const uint8_t *src = ctx->local_address;
    volatile uint8_t *dest = ctx->remote_address;

    const uint8_t *end = src + bytes;
    while (src < end) {
        *dest++ = *src++;
    }
}

static inline void memcpy_32_chunks_op(int i, void *vctx, int bytes)
{
    (void)i;  /* iteration index unused */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    const uint32_t *src = (const uint32_t *)ctx->local_address;
    volatile uint32_t *dest = (volatile uint32_t *)ctx->remote_address;

    const uint32_t *end = src + (bytes / 4);
    while (src < end) {
        *dest++ = *src++;
    }
}

static inline void memcpy_64_chunks_op(int i, void *vctx, int bytes)
{
    (void)i;  /* iteration index unused */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    const uint64_t *src = (const uint64_t *)ctx->local_address;
    volatile uint64_t *dest = (volatile uint64_t *)ctx->remote_address;

    const uint64_t *end = src + (bytes / 8);
    while (src < end) {
        *dest++ = *src++;
    }
}

static inline void memcpy_nonvol_64_chunks_op(int i, void *vctx, int bytes)
{
    (void)i;  /* iteration index unused */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    const uint64_t *src = (const uint64_t *)ctx->local_address;
    uint64_t *dest = (uint64_t *)ctx->remote_address;

    const uint64_t *end = src + (bytes / 8);
    while (src < end) {
        *dest++ = *src++;
    }
}

static inline void memcpy_chunks_generic(volatile void *dst, const void *src,
                                         size_t len, size_t elem_size)
{
    size_t n = len / elem_size;
    const char *s = src;
    volatile char *d = dst;

    for (size_t i = 0; i < n; ++i) {
        for (size_t b = 0; b < elem_size; ++b)
            d[b] = s[b];
        d += elem_size;
        s += elem_size;
    }
}

static inline void memcpy_chunks(int i, void *vctx, int bytes) {
    (void)i;  /* iteration index unused */
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;

    memcpy_chunks_generic(ctx->remote_address, ctx->local_address, bytes, ctx->trans_size);
}


/* AVX2 non-temporal copy to remote mapped memory.
   - Aligns dest to 64B to avoid split lines
   - Uses _mm256_stream_si256 (32B NT stores) in a 64B unrolled loop
   - Light source prefetch
   - _mm_sfence at the end to complete NT stores
*/
static void memcpy_avx2_load_stream_store(int i, void *vctx, int bytes)
{
    (void)i;
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
#if defined(__AVX2__)
    const uint8_t *src = (const uint8_t *) ctx->local_address;
    uint8_t *dst = (uint8_t *)(uintptr_t) ctx->remote_address;

    int n = bytes;
    while (n >= 64) {
        __m256i a = _mm256_load_si256((const __m256i*)(src));
        __m256i b = _mm256_load_si256((const __m256i*)(src + 32));
        _mm256_stream_si256((__m256i*)(dst), a);
        _mm256_stream_si256((__m256i*)(dst + 32), b);
        src += 64; dst += 64; n -= 64;
    }
    if (n >= 32) {
        __m256i v = _mm256_load_si256((const __m256i*)src);
        _mm256_stream_si256((__m256i*)dst, v);
        src += 32; dst += 32; n -= 32;
    }
    if (n >= 16) {
        __m128i v16 = _mm_load_si128((const __m128i*)src);
        _mm_stream_si128((__m128i*)dst, v16);
        src += 16; dst += 16; n -= 16;
    }
    while (n >= 8) { *(uint64_t*)dst = *(const uint64_t*)src; dst+=8; src+=8; n-=8; }
    while (n--) { *dst++ = *src++; }
#else
    printf("ERROR: NO AVX2 support\n");
#endif
}

static void memcpy_avx2_loadu_storeu(int i, void *vctx, int bytes)
{
    (void)i;
    memcpy_ctx_t *ctx = (memcpy_ctx_t *)vctx;
#if defined(__AVX2__)
    const uint8_t *src = (const uint8_t *) ctx->local_address;
    uint8_t *dst = (uint8_t *)(uintptr_t) ctx->remote_address;

    int n = bytes;
    while (n >= 64) {
        __m256i a = _mm256_loadu_si256((const __m256i*)(src));
        __m256i b = _mm256_loadu_si256((const __m256i*)(src + 32));
        _mm256_storeu_si256((__m256i*)(dst), a);
        _mm256_storeu_si256((__m256i*)(dst + 32), b);
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
    while (n--) { *dst++ = *src++; }
#else
    printf("ERROR: NO AVX2 support\n");
#endif
}

static void avx2_fence_cb(void *ctx)
{
    (void)ctx;
#if defined(__AVX2__)
    _mm_sfence();
#endif
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <remote node id> <bytes>\n", argv[0]);
        exit(1);
    }

    int remote_node_id = atoi(argv[1]);
    int bytes = atoi(argv[2]);

    printf("Remote node id: %d\n", remote_node_id);
    printf("Size: %d\n", bytes);

    sci_desc_t sd;
    sci_error_t error;
    void* local_address;
    int i;
    char *malloc_x = malloc(bytes + 63);
    while ((size_t)malloc_x & 0x3f) {
        malloc_x++;
    }
    local_address = (void *) malloc_x;
    for (i=0; i<bytes; ++i) malloc_x[i] = i & 255;

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
                            bytes, NO_ADDRESS_HINT, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) return 1;
    SEOE(SCICreateMapSequence, remote_map, &remote_sequence, NO_FLAGS);

    /* Prepare context for our op */
    memcpy_ctx_t ctx = {
        .remote_sequence = remote_sequence,
        .local_address   = local_address,
        .remote_map      = remote_map,
        .bytes           = bytes,
        .remote_address  = remote_address,
        .trans_size      = 0
    };


#if defined(__AVX512F__)
    printf("AVX512 support detected!\n");
#else
    printf("WARN: NO AVX512 support detected\n");
#endif

    /* Warm-up using the same op */
    for (int i = 0; i < WULOOPS; i++) {
        scicopy_op(i, &ctx, bytes);
    }
    printf("Warmed up, running only 64:\n");

    for (int csize = 64; csize <= bytes; csize *= 2) {
        run_benchmark(memcpy_64_chunks_op, &ctx, csize, "memcpy_64_chunks_op", avx2_fence_cb);
    }

    printf("8b:\n");
    for (int csize = 64; csize <= bytes; csize *= 2) {
        run_benchmark(memcpy_8_chunks_op, &ctx, csize, "memcpy_8_chunks_op", avx2_fence_cb);
    }
    printf("Running 2 32s:\n");

    for (int csize = 64; csize <= bytes; csize *= 2) {
        run_benchmark(memcpy_32_chunks_op, &ctx, csize, "memcpy_32_chunks_op", avx2_fence_cb);
        run_benchmark(memcpy_32_chunks_op, &ctx, csize, "memcpy_32_chunks_op", avx2_fence_cb);
    }

    printf("Running 2 64s:\n");

    for (int csize = 64; csize <= bytes; csize *= 2) {
        run_benchmark(memcpy_64_chunks_op, &ctx, csize, "memcpy_64_chunks_op", avx2_fence_cb);
        run_benchmark(memcpy_64_chunks_op, &ctx, csize, "memcpy_64_chunks_op", avx2_fence_cb);
    }

    printf("Running 32 and then 64:\n");

    for (int csize = 64; csize <= bytes; csize *= 2) {
        run_benchmark(memcpy_32_chunks_op, &ctx, csize, "memcpy_32_chunks_op", avx2_fence_cb);
        run_benchmark(memcpy_64_chunks_op, &ctx, csize, "memcpy_64_chunks_op", avx2_fence_cb);
    }


    printf("Running 64 and then 32:\n");

    for (int csize = 64; csize <= bytes; csize *= 2) {
        run_benchmark(memcpy_64_chunks_op, &ctx, csize, "memcpy_64_chunks_op", avx2_fence_cb);
        run_benchmark(memcpy_32_chunks_op, &ctx, csize, "memcpy_32_chunks_op", avx2_fence_cb);
    }

    printf("GENERIC:\n");
    for (int csize = 64; csize <= bytes; csize *= 2) {
        for (int tsize = 1; tsize <= 512; tsize *= 2) {
            ctx.trans_size = tsize;
            run_benchmark(memcpy_chunks, &ctx, csize, "memcpy_%s_chunks_generic", avx2_fence_cb);
        }
    }

    printf("Majloop:\n");

    for (int csize = 8; csize <= bytes; csize *= 2) {
        /* Timed benchmark with op callback */
        run_benchmark(scicopy_op, &ctx, csize, "scicopy_op", NULL);
        
        run_benchmark(scicopy_two_halves_op, &ctx, csize, "scicopy_two_halves_op", NULL);

        run_benchmark(memcopy_op, &ctx, csize, "memcopy_op", avx2_fence_cb);

        run_benchmark(memcpy_avx2_load_stream_store, &ctx, csize, "memcpy_avx2_load_stream_store", avx2_fence_cb);

        run_benchmark(memcpy_avx2_loadu_storeu, &ctx, csize, "memcpy_avx2_loadu_storeu", avx2_fence_cb);

        if (csize >= 32) {
            run_benchmark(memcpy_32_chunks_op, &ctx, csize, "memcpy_32_chunks_op", avx2_fence_cb);
        }

        if (csize >= 64) {
            run_benchmark(memcpy_64_chunks_op, &ctx, csize, "memcpy_64_chunks_op", avx2_fence_cb);
        
            run_benchmark(memcpy_nonvol_64_chunks_op, &ctx, csize, "memcpy_nonvol_64_chunks_op", avx2_fence_cb);
        }
    }

    SEOE(SCIRemoveSequence, remote_sequence, NO_FLAGS);
    SEOE(SCIUnmapSegment, remote_map, NO_FLAGS);
    SEOE(SCIDisconnectSegment, remote_segment, NO_FLAGS);
    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
    return 0;
}

