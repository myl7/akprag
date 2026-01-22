// SPDX-License-Identifier: Apache-2.0

#include <fss/group.h>
#include <string.h>
#include "../utils.h"

#if kLambda != 16
#error "kLambda must be 16 for u128_le group"
#endif

#define kPrime 18446744073709551557ull

FSS_CUDA_HOST_DEVICE void group_add(uint8_t *val, const uint8_t *rhs) {
  uint64_t *val64 = (uint64_t *)val;
  if (*val64 >= kPrime) *val64 -= kPrime;
  const uint64_t *rhs64_ptr = (const uint64_t *)rhs;
  uint64_t rhs64 = *rhs64_ptr;
  if (rhs64 >= kPrime) rhs64 -= kPrime;
  if (*val64 >= kPrime - rhs64) *val64 += rhs64 - kPrime;
  else *val64 += rhs64;
  memset(val + 8, 0, 8);
}

FSS_CUDA_HOST_DEVICE void group_neg(uint8_t *val) {
  uint64_t *val64 = (uint64_t *)val;
  if (*val64 >= kPrime) *val64 -= kPrime;
  if (*val64 == 0) return;
  *val64 = kPrime - *val64;
  memset(val + 8, 0, 8);
}

FSS_CUDA_HOST_DEVICE void group_zero(uint8_t *val) {
  memset(val, 0, 8);
}
