// For real-time extensions
#define _POSIX_C_SOURCE 199309L

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <fss/dcf.h>
#include <omp.h>

#define kSeed 114514
#define kAlphaBitlen 64
#define kAlphaBytelen 8
#define kN 100000

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

// Get int from little-endian kAlphaBitlen bits
static uint64_t get_alpha_int_le(uint8_t *alpha) {
  uint64_t val = 0;
  for (int i = 0; i < kAlphaBytelen; i++) {
    val |= ((uint64_t)alpha[i]) << (i * 8);
  }
#if kAlphaBitlen < 64
  val &= ((1ULL << kAlphaBitlen) - 1);
#endif
  return val;
}

int main() {
  assert((kAlphaBitlen + 7) / 8 == kAlphaBytelen);
  assert(kAlphaBytelen <= 8);
  srand(kSeed);
  double t;
  int iter_num;
  printf("OpenMP thread num: %d\n", omp_get_max_threads());
  printf("Alpha bitlen: %d\n", kAlphaBitlen);
  printf("Lambda (B): %d\n", kLambda);

  // Init PRG
  uint8_t *keys = (uint8_t *)malloc(4 * kLambda);
  assert(keys != NULL);
  gen_rand_bytes(keys, 4 * kLambda);
  prg_init(keys, 4 * kLambda);
  free(keys);

  // Sample s0s
  uint8_t *s0s = (uint8_t *)malloc(kLambda * 2);
  assert(s0s != NULL);
  gen_rand_bytes(s0s, kLambda * 2);

  // Prepare comparison function
  uint8_t alpha[kAlphaBytelen];
  gen_rand_bytes(alpha, kAlphaBytelen);
  uint64_t alpha_int = get_alpha_int_le(alpha);
  Bits alpha_bits = {alpha, kAlphaBitlen};

  uint8_t *beta = (uint8_t *)malloc(kLambda);
  assert(beta != NULL);
  memset(beta, 0, kLambda);
  uint64_t beta_val;
  gen_rand_bytes((uint8_t *)&beta_val, 8);
  memcpy(beta, &beta_val, 8);

  Point p = {alpha_bits, beta};
  CmpFunc cf = {p, kLtAlpha};

  // Alloc sbuf and k
  uint8_t *sbuf = (uint8_t *)malloc(kLambda * 10);
  assert(sbuf != NULL);

  Key k;
  k.cw_np1 = (uint8_t *)malloc(kLambda);
  assert(k.cw_np1 != NULL);
  k.cws = (uint8_t *)malloc(kDcfCwLen * kAlphaBitlen);
  assert(k.cws != NULL);

  // DCF gen
  iter_num = 100000;
  t = get_time();
  for (int i = 0; i < iter_num; i++) {
    memcpy(sbuf, s0s, kLambda * 2);
    dcf_gen(k, cf, sbuf);
  }
  printf("dcf_gen (us): %lf\n", (get_time() - t) / iter_num * 1e6);

  free(sbuf);

  int thread_num = omp_get_max_threads();
  uint8_t *sbufs = (uint8_t *)malloc(kLambda * 6 * thread_num);
  assert(sbufs != NULL);

  uint64_t *xs = (uint64_t *)malloc(kN * sizeof(uint64_t));
  assert(xs != NULL);
  for (int i = 0; i < kN; i++) {
    gen_rand_bytes((uint8_t *)&xs[i], 8);
  }

  // DCF eval
  t = get_time();
#pragma omp parallel for
  for (int i = 0; i < kN; i++) {
    int tid = omp_get_thread_num();
    uint8_t *sbuf = sbufs + tid * kLambda * 6;

    memcpy(sbuf, s0s, kLambda);
    Bits x_bits = {(uint8_t *)&xs[i], kAlphaBitlen};
    dcf_eval(sbuf, 0, k, x_bits);
  }
  double t_elapsed = get_time() - t;
  printf("dcf_eval (us): %lf\n", t_elapsed / kN * 1e6);

  free(sbufs);
  free(xs);

  // Cleanup
  prg_free();
  free(s0s);
  free(beta);
  free(k.cw_np1);
  free(k.cws);
  return 0;
}
