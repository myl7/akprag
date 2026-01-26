// SPDX-License-Identifier: Apache-2.0

// For real-time extensions
#define _POSIX_C_SOURCE 199309L

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <fss/dcf.h>
#include <fss/group.h>
#include <omp.h>

#define kPrime 18446744073709551557ull
#define kSeed 114514
#define kAlphaBitlen 64
#define kAlphaBytelen 8
#define kN 1048576

// Use __int128 for easier modular arithmetic mod p where p ~ 2^64
typedef unsigned __int128 uint128_t;

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

// Convert u64 to group element (little endian, padded) (Assuming kLambda=16)
static void u64_to_group(uint8_t *g, uint64_t v) {
    memset(g, 0, kLambda);
    memcpy(g, &v, 8);
}

static uint64_t group_to_u64(const uint8_t *g) {
    uint64_t v;
    memcpy(&v, g, 8); // Assumes val < p stored in first 8 bytes
    return v;
}

int main() {
  srand(kSeed);
  printf("Cmp Protocol Benchmark\n");
  printf("Lambda (B): %d\n", kLambda);

  // Init PRG
  uint8_t *keys = (uint8_t *)malloc(4 * kLambda);
  gen_rand_bytes(keys, 4 * kLambda);
  prg_init(keys, 4 * kLambda);
  free(keys);

  int iter_num = kN;

  // Buffers for keys
  Key key_l, key_r;
  // Allocation
  key_l.cw_np1 = (uint8_t*)malloc(kLambda); key_l.cws = (uint8_t*)malloc(kDcfCwLen * kAlphaBitlen);
  key_r.cw_np1 = (uint8_t*)malloc(kLambda); key_r.cws = (uint8_t*)malloc(kDcfCwLen * kAlphaBitlen);

  uint8_t *sbuf_l = (uint8_t*)malloc(kLambda * 10);
  uint8_t *sbuf_r = (uint8_t*)malloc(kLambda * 10);

  int gen_iter_num = 1;

  // Gen Bench
  printf("Benchmarking Cmp.Gen...\n");
  double t = get_time();
  for (int i=0; i < gen_iter_num; i++) {
        uint64_t xl = get_rand_field();
        uint64_t xr = get_rand_field();
        uint64_t r = get_rand_field(); // Dealer r

        uint64_t xl_p = add_mod_p(xl, r);
        uint64_t xr_p = add_mod_p(xr, r);

        // Gen DCF L: x < xl_p ? p-1 : 0
        uint64_t pl_val = kPrime - 1;
        uint8_t pl[kLambda]; u64_to_group(pl, pl_val);
        Bits alpha_l = {(uint8_t*)&xl_p, kAlphaBitlen};
        CmpFunc cf_l = {{alpha_l, pl}, kLtAlpha};

        // Gen DCF R: x < xr_p ? 1 : 0
        uint64_t pr_val = 1;
        uint8_t pr[kLambda]; u64_to_group(pr, pr_val);
        Bits alpha_r = {(uint8_t*)&xr_p, kAlphaBitlen};
        CmpFunc cf_r = {{alpha_r, pr}, kLtAlpha};

        // Seeds for this iter (simulating random)
        gen_rand_bytes(sbuf_l, 2*kLambda);
        gen_rand_bytes(sbuf_r, 2*kLambda);

        dcf_gen(key_l, cf_l, sbuf_l);
        dcf_gen(key_r, cf_r, sbuf_r);

        uint64_t w = (xl_p > xr_p) ? 1 : 0;
        uint64_t w0 = get_rand_field();
        // uint64_t w1 = sub_mod_p(w, w0); // Not used in Gen bench but part of protocol
  }
  printf("Cmp.Gen time (us/op): %lf\n", (get_time() - t) / gen_iter_num * 1e6);

  // Eval Bench
  printf("Benchmarking Cmp.Eval...\n");

  // Setup fixed keys for Eval
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

  uint8_t s0s_l[2*kLambda]; gen_rand_bytes(s0s_l, 2*kLambda);
  uint8_t s0s_r[2*kLambda]; gen_rand_bytes(s0s_r, 2*kLambda);

  memcpy(sbuf_l, s0s_l, 2*kLambda);
  dcf_gen(key_l, cf_l, sbuf_l);

  memcpy(sbuf_r, s0s_r, 2*kLambda);
  dcf_gen(key_r, cf_r, sbuf_r);

  uint64_t w = (xl_p > xr_p) ? 1 : 0;
  uint64_t w0 = get_rand_field();

  // Pre-generate random inputs and allocate thread-local buffers
  int thread_num = omp_get_max_threads();
  uint8_t *sbufs_l = (uint8_t *)malloc(kLambda * 10 * thread_num);
  uint8_t *sbufs_r = (uint8_t *)malloc(kLambda * 10 * thread_num);
  uint64_t *xs_eval = (uint64_t *)malloc(iter_num * sizeof(uint64_t));
  if (!sbufs_l || !sbufs_r || !xs_eval) {
      perror("malloc failed");
      return 1;
  }

  for (int i = 0; i < iter_num; i++) {
      xs_eval[i] = get_rand_field();
  }

  t = get_time();
#pragma omp parallel for
  for (int i=0; i < iter_num; i++) {
     int tid = omp_get_thread_num();
     uint8_t *sbuf_l_local = sbufs_l + tid * kLambda * 10;
     uint8_t *sbuf_r_local = sbufs_r + tid * kLambda * 10;

     uint64_t x = xs_eval[i];
     uint64_t z = add_mod_p(x, r);
     Bits z_bits = {(uint8_t*)&z, kAlphaBitlen};

     // Simulating P0 Eval
     memcpy(sbuf_l_local, s0s_l, kLambda); // Reset seed for P0
     memcpy(sbuf_r_local, s0s_r, kLambda); // Reset seed for P0

     dcf_eval(sbuf_l_local, 0, key_l, z_bits);
     dcf_eval(sbuf_r_local, 0, key_r, z_bits);

     uint64_t y_l = group_to_u64(sbuf_l_local);
     uint64_t y_r = group_to_u64(sbuf_r_local);
     uint64_t res = add_mod_p(add_mod_p(y_l, y_r), w0);
     (void)res;
  }
  printf("Cmp.Eval (one party) time (all) (ms/op): %lf\n", (get_time() - t) * 1e3);

  free(sbufs_l);
  free(sbufs_r);
  free(xs_eval);

  free(key_l.cw_np1); free(key_l.cws);
  free(key_r.cw_np1); free(key_r.cws);
  free(sbuf_l); free(sbuf_r);

  return 0;
}
