/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define KMP_LOCK_FREE 0
#define KMP_LOCK_BUSY 1

void __builtin_IB_kmp_acquire_lock(int *lock) {
  volatile atomic_uint *lck = (volatile atomic_uint *)lock;
  uint expected = KMP_LOCK_FREE;
  while (atomic_load_explicit(lck, memory_order_relaxed) != KMP_LOCK_FREE ||
         !atomic_compare_exchange_strong_explicit(lck, &expected, KMP_LOCK_BUSY, memory_order_acquire,
                                                  memory_order_relaxed)) {
    expected = KMP_LOCK_FREE;
  }
}

void __builtin_IB_kmp_release_lock(int *lock) {
  volatile atomic_uint *lck = (volatile atomic_uint *)lock;
  atomic_store_explicit(lck, KMP_LOCK_FREE, memory_order_release);
}

#undef KMP_LOCK_FREE
#undef KMP_LOCK_BUSY
