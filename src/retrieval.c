// SPDX-License-Identifier: Apache-2.0
#define _POSIX_C_SOURCE 199309L

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <omp.h>
#include <fss/dcf.h>
#include <fss/group.h>

#define kPrime 18446744073709551557ull
#define kDim 1024
#define kN 131072  // Number of documents
#define kStep 13    // Number of binary search steps
#define kSeed 114514
#define kAlphaBitlen 64

typedef unsigned __int128 uint128_t;

// --- Helper Functions ---

static inline double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec * 1.0e-9;
}

static void gen_rand_bytes(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buf[i] = rand() & 0xFF;
    }
}

static uint64_t get_rand_field() {
    uint64_t r;
    uint8_t buf[8];
    gen_rand_bytes(buf, 8);
    memcpy(&r, buf, 8);
    if (r >= kPrime) return r - kPrime;
    return r;
}

static uint64_t add_mod_p(uint64_t a, uint64_t b) {
    return (uint64_t)(((uint128_t)a + b) % kPrime);
}

static uint64_t sub_mod_p(uint64_t a, uint64_t b) {
    return (uint64_t)(((uint128_t)a + kPrime - b) % kPrime);
}

static uint64_t mul_mod_p(uint64_t a, uint64_t b) {
    return (uint64_t)(((uint128_t)a * b) % kPrime);
}

static void u64_to_group(uint8_t *g, uint64_t v) {
    memset(g, 0, kLambda);
    memcpy(g, &v, 8);
}

static uint64_t group_to_u64(const uint8_t *g) {
    uint64_t v;
    memcpy(&v, g, 8);
    return v;
}

// --- Dot Product Data ---
// Only allocating what is needed for the "kernel" simulation
uint64_t share_a_0[kDim], share_a_1[kDim];
uint64_t share_b_0[kDim], share_b_1[kDim];
uint64_t share_x_0[kDim], share_x_1[kDim];
uint64_t share_y_0[kDim], share_y_1[kDim];
uint64_t share_z_0[kDim], share_z_1[kDim];

void setup_dotprod_data() {
    for (int k = 0; k < kDim; ++k) {
        uint64_t ak = get_rand_field();
        uint64_t bk = get_rand_field();
        uint64_t xk = get_rand_field();
        uint64_t yk = get_rand_field();
        uint64_t zk = mul_mod_p(xk, yk);

        share_a_0[k] = get_rand_field();
        share_a_1[k] = sub_mod_p(ak, share_a_0[k]);
        share_b_0[k] = get_rand_field();
        share_b_1[k] = sub_mod_p(bk, share_b_0[k]);
        share_x_0[k] = get_rand_field();
        share_x_1[k] = sub_mod_p(xk, share_x_0[k]);
        share_y_0[k] = get_rand_field();
        share_y_1[k] = sub_mod_p(yk, share_y_0[k]);
        share_z_0[k] = get_rand_field();
        share_z_1[k] = sub_mod_p(zk, share_z_0[k]);
    }
}

// Global keys for CMP
Key key_l, key_r;
// Buffers for keys (allocated in main)

// Main Protocol Benchmark
int main() {
    srand(kSeed);
    printf("Retrieval Protocol Benchmark\n");
    printf("N (Docs): %d\n", kN);
    printf("Dim: %d\n", kDim);
    printf("Steps: %d\n", kStep);

    // --- Init ---
    setup_dotprod_data();

    uint8_t *keys = (uint8_t *)malloc(4 * kLambda);
    gen_rand_bytes(keys, 4 * kLambda);
    prg_init(keys, 4 * kLambda);
    free(keys);

    // Alloc keys
    key_l.cw_np1 = (uint8_t*)malloc(kLambda); key_l.cws = (uint8_t*)malloc(kDcfCwLen * kAlphaBitlen);
    key_r.cw_np1 = (uint8_t*)malloc(kLambda); key_r.cws = (uint8_t*)malloc(kDcfCwLen * kAlphaBitlen);

    // Thread local buffers for Eval
    int thread_num = omp_get_max_threads();
    uint8_t *sbufs_l = (uint8_t *)malloc(kLambda * 10 * thread_num);
    uint8_t *sbufs_r = (uint8_t *)malloc(kLambda * 10 * thread_num);

    // Gen Buffer
    uint8_t *sbuf_l_gen = (uint8_t*)malloc(kLambda * 10);
    uint8_t *sbuf_r_gen = (uint8_t*)malloc(kLambda * 10);

    // Dummy inputs
    uint64_t *xs_eval = (uint64_t *)malloc(kN * sizeof(uint64_t));
    for(int i=0; i<kN; ++i) xs_eval[i] = get_rand_field();

    printf("Starting Benchmark...\n");
    double start_total = get_time();

    // Pre-compute Party 1's value to exclude them from timing
    uint64_t *d1_buf = (uint64_t*)malloc(kDim * sizeof(uint64_t));
    uint64_t *e1_buf = (uint64_t*)malloc(kDim * sizeof(uint64_t));
    for(int k=0; k<kDim; ++k) {
        d1_buf[k] = sub_mod_p(share_a_1[k], share_x_1[k]);
        e1_buf[k] = sub_mod_p(share_b_1[k], share_y_1[k]);
    }

    // 1. Servers compute [d_j] = [v_p . v_x_j] for all docs (Dot Product)
    // Runs N dot products. logic from dotprod.c
    // "1 dotprod.c" -> dotprod.c does kN iterations.

    #pragma omp parallel for
    for (int iter = 0; iter < kN; ++iter) {
        uint64_t local_d[kDim];
        uint64_t local_e[kDim];

        for (int k = 0; k < kDim; ++k) {
            local_d[k] = sub_mod_p(share_a_0[k], share_x_0[k]);
            local_e[k] = sub_mod_p(share_b_0[k], share_y_0[k]);
        }

        uint64_t final_res_share = 0;
        for (int k = 0; k < kDim; ++k) {
            uint64_t d1 = d1_buf[k];
            uint64_t e1 = e1_buf[k];
            uint64_t dk = add_mod_p(local_d[k], d1);
            uint64_t ek = add_mod_p(local_e[k], e1);

            uint64_t t1 = mul_mod_p(ek, share_x_0[k]);
            uint64_t t2 = mul_mod_p(dk, share_y_0[k]);
            uint64_t ck = add_mod_p(share_z_0[k], t1);
            ck = add_mod_p(ck, t2);

            final_res_share = add_mod_p(final_res_share, ck);
        }
        // Store [d_j] (not actually storing to save memory/complexity in bench, assume done)
        volatile uint64_t sink = final_res_share; (void)sink;
    }
    free(d1_buf); free(e1_buf);

    // Loop
    double gen_time_total = 0;
    for (int s = 0; s < kStep; ++s) {
        // User Gen (Simulated)
        // Cmp.Gen([d_k + delta, p))
        // We do 1 Gen.
        double t_gen_start = get_time();
        {
            uint64_t r = get_rand_field();
            uint64_t xl = get_rand_field();
            uint64_t xr = get_rand_field();
            uint64_t xl_p = add_mod_p(xl, r);
            uint64_t xr_p = add_mod_p(xr, r);

            uint64_t pl_val = kPrime - 1;
            uint8_t pl[kLambda]; u64_to_group(pl, pl_val);
            Bits alpha_l = {(uint8_t*)&xl_p, kAlphaBitlen};
            CmpFunc cf_l = {{alpha_l, pl}, kLtAlpha};

            uint64_t pr_val = 1;
            uint8_t pr[kLambda]; u64_to_group(pr, pr_val);
            Bits alpha_r = {(uint8_t*)&xr_p, kAlphaBitlen};
            CmpFunc cf_r = {{alpha_r, pr}, kLtAlpha};

            gen_rand_bytes(sbuf_l_gen, 2*kLambda);
            gen_rand_bytes(sbuf_r_gen, 2*kLambda);

            dcf_gen(key_l, cf_l, sbuf_l_gen);
            dcf_gen(key_r, cf_r, sbuf_r_gen);
        }
        gen_time_total += get_time() - t_gen_start;

        // Servers Eval for all docs
        // N ops
        #pragma omp parallel for
        for (int i = 0; i < kN; ++i) {
            int tid = omp_get_thread_num();
            uint8_t *sbuf_l_local = sbufs_l + tid * kLambda * 10;
            uint8_t *sbuf_r_local = sbufs_r + tid * kLambda * 10;

            uint64_t x = xs_eval[i];
            Bits z_bits = {(uint8_t*)&x, kAlphaBitlen}; // Assume x is masked properly

            // Use the seed from Gen (simulated propagation)
            memcpy(sbuf_l_local, sbuf_l_gen, kLambda);
            memcpy(sbuf_r_local, sbuf_r_gen, kLambda);

            dcf_eval(sbuf_l_local, 0, key_l, z_bits);
            dcf_eval(sbuf_r_local, 0, key_r, z_bits);

            volatile uint64_t y_l = group_to_u64(sbuf_l_local);
            volatile uint64_t y_r = group_to_u64(sbuf_r_local);
            (void)y_l; (void)y_r;
        }

        // Servers return [c] (sum) - ignored in benchmark as lightweight add
    }

    // Post-Loop: Cmp.Eval (N ops) for results + Cmp.Eval (1 op) for check
    // 1. Cmp.Eval([d_{c,j}])
    {
         // Assume we use the last generated key or a fixed one (doesn't matter for perf)
        #pragma omp parallel for
        for (int i = 0; i < kN; ++i) {
            int tid = omp_get_thread_num();
            uint8_t *sbuf_l_local = sbufs_l + tid * kLambda * 10;
            uint8_t *sbuf_r_local = sbufs_r + tid * kLambda * 10;

            uint64_t x = xs_eval[i];
            Bits z_bits = {(uint8_t*)&x, kAlphaBitlen};

            memcpy(sbuf_l_local, sbuf_l_gen, kLambda);
            memcpy(sbuf_r_local, sbuf_r_gen, kLambda);

            dcf_eval(sbuf_l_local, 0, key_l, z_bits);
            dcf_eval(sbuf_r_local, 0, key_r, z_bits);
        }
    }

    // 2. Cmp.Eval([c]) - 1 op
    {
        uint8_t *sbuf_l_local = sbufs_l; // main thread buffer
        uint8_t *sbuf_r_local = sbufs_r;
        uint64_t x = get_rand_field();
        Bits z_bits = {(uint8_t*)&x, kAlphaBitlen};

        memcpy(sbuf_l_local, sbuf_l_gen, kLambda);
        memcpy(sbuf_r_local, sbuf_r_gen, kLambda);

        dcf_eval(sbuf_l_local, 0, key_l, z_bits);
        dcf_eval(sbuf_r_local, 0, key_r, z_bits);
    }

    double end_total = get_time();
    printf("Total Time: %lf ms\n", (end_total - start_total - gen_time_total) * 1e3);

    // Cleanup
    free(key_l.cw_np1); free(key_l.cws);
    free(key_r.cw_np1); free(key_r.cws);
    free(sbufs_l); free(sbufs_r);
    free(sbuf_l_gen); free(sbuf_r_gen);
    free(xs_eval);

    return 0;
}
