// SPDX-License-Identifier: Apache-2.0

// For real-time extensions
#define _POSIX_C_SOURCE 199309L

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <omp.h>

#define kPrime 18446744073709551557ull
#define kDim 1024
#define kN 1048576  // Iterations for benchmark

typedef unsigned __int128 uint128_t;

static inline double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec * 1.0e-9;
}

static void gen_rand_bytes(uint8_t *buf, size_t len) {
    // Simple rand for benchmark setup
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

// Global "const" vectors and triples
uint64_t vec_a[kDim];  // Secret usually, but we need truth for verifying or just input shares
uint64_t vec_b[kDim];

// Shares for Party 0 and 1
uint64_t share_a_0[kDim], share_a_1[kDim];
uint64_t share_b_0[kDim], share_b_1[kDim];

// Triples
uint64_t share_x_0[kDim], share_x_1[kDim];
uint64_t share_y_0[kDim], share_y_1[kDim];
uint64_t share_z_0[kDim], share_z_1[kDim];

// Open values (simulated communication)
uint64_t d_open[kDim];
uint64_t e_open[kDim];

void setup_data() {
    for (int k = 0; k < kDim; ++k) {
        // True values
        uint64_t ak = get_rand_field();
        uint64_t bk = get_rand_field();
        uint64_t xk = get_rand_field();
        uint64_t yk = get_rand_field();
        uint64_t zk = mul_mod_p(xk, yk);

        // Shares for a
        share_a_0[k] = get_rand_field();
        share_a_1[k] = sub_mod_p(ak, share_a_0[k]);

        // Shares for b
        share_b_0[k] = get_rand_field();
        share_b_1[k] = sub_mod_p(bk, share_b_0[k]);

        // Shares for x
        share_x_0[k] = get_rand_field();
        share_x_1[k] = sub_mod_p(xk, share_x_0[k]);

        // Shares for y
        share_y_0[k] = get_rand_field();
        share_y_1[k] = sub_mod_p(yk, share_y_0[k]);

        // Shares for z
        share_z_0[k] = get_rand_field();
        share_z_1[k] = sub_mod_p(zk, share_z_0[k]);
    }
}

int main() {
    srand(time(NULL));
    setup_data();

    printf("Benchmarking Dot Product (Server 0 view)...\n");
    printf("Dimension: %d\n", kDim);
    printf("Iterations: %d\n", kN);

    double start = get_time();

#pragma omp parallel for
    for (int iter = 0; iter < kN; ++iter) {
        // --- Online Stage (Simulating Party 0) ---
        // 1. Compute shares of d and e: [d] = [a] - [x], [e] = [b] - [y]

        // Using temporary arrays to simulate "publishing" step separation
        // In reality, this loop would engage network IO every step or batched.
        // We assume batched for the sake of calculation of d_open later,
        // but timing includes the arithmetic.

        uint64_t local_d[kDim];
        uint64_t local_e[kDim];

        for (int k = 0; k < kDim; ++k) {
            local_d[k] = sub_mod_p(share_a_0[k], share_x_0[k]);
            local_e[k] = sub_mod_p(share_b_0[k], share_y_0[k]);
        }

        // 2. Publish d_k, e_k
        // Simulation: We assume we receive Party 1's share instantly (or just have access to it)
        // Ideally we shouldn't count the time to generate Party 1's share,
        // but we must count the time to reconstructions (adds).

        // Generate P1 shares (cheat: do it outside measurement or minimal cost?)
        // To be fair, let's pre-calculate P1's d/e or calculate them on fly but exclude time?
        // Actually, the request says "evaluate the time of one server".
        // So we only time P0's ops. But P0 must perform the reconstruction add: d = d0 + d1.

        // Let's pretend we "received" d1, e1.
        // We calculate d[k] = local_d[k] + d1[k].
        // I will calculate d1[k] right here for correctness but it adds slight overhead.
        // Given this is a microbenchmark, overhead might matter.
        // But the main cost is modular mul/add.

        uint64_t final_res_share = 0;

        for (int k = 0; k < kDim; ++k) {
            // "Receive" operations (simulating having d1, e1 ready to add)
            // d1 = share_a_1[k] - share_x_1[k]
            // e1 = share_b_1[k] - share_y_1[k]
            uint64_t d1 = sub_mod_p(share_a_1[k], share_x_1[k]);
            uint64_t e1 = sub_mod_p(share_b_1[k], share_y_1[k]);

            // Actual op to time: Reconstruction
            uint64_t dk = add_mod_p(local_d[k], d1);
            uint64_t ek = add_mod_p(local_e[k], e1);

            // 3. Compute [c_k]_i
            // [c_k]_0 = [z_k]_0 + ek * [x_k]_0 + dk * [y_k]_0 + 0 * dk * ek

            uint64_t t1 = mul_mod_p(ek, share_x_0[k]);
            uint64_t t2 = mul_mod_p(dk, share_y_0[k]);

            uint64_t ck = add_mod_p(share_z_0[k], t1);
            ck = add_mod_p(ck, t2);

            // iterm: i * dk * ek. For Party 0, i=0, so it's 0.
            // if (party_id == 1) ck = add_mod_p(ck, mul_mod_p(dk, ek));

            // 4. Accumulate result
            final_res_share = add_mod_p(final_res_share, ck);
        }

        // Prevent opt out
        if (final_res_share == 0xDEADBEEF) printf("Startled\n");
    }

    double end = get_time();
    double total_time = end - start;

    printf("Dot Product (dim=%d): %lf ms\n", kDim, total_time * 1e3);

    return 0;
}
