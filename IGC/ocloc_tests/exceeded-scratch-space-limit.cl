/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: dg2-supported
// RUN: not ocloc compile -file %s -options "-cl-opt-disable -igc_opts 'MaxPerThreadScratchSpaceOverride=1024'" -device dg2 2>&1 | FileCheck %s

// This test verifies that the compiler emits a proper error when a kernel's
// scratch space usage exceeds hardware capabilities, instead of silently
// dropping the kernel from the final binary.
//
// Test Setup:
// - Uses MaxPerThreadScratchSpaceOverride=1024 to simulate hardware with only
//   1024 bytes of available scratch space (much lower than real DG2 limits)
// - Compiles with -cl-opt-disable to prevent recompilation and function inlining
// - Uses intel_reqd_sub_group_size(32) to prevent the compiler from reducing
//   sub-group size, which could eliminate register pressure and spills
//
// The spilling occurs in a non-kernel function (this_is_function_with_spills)
// that creates 16 local variables through macro expansion and performs multiple
// operations to force register pressure. The artificial 1024-byte limit is
// necessary because creating a kernel that naturally exceeds real hardware
// capabilities would take too long to compile for a LIT test.

// CHECK: error: total scratch space exceeds HW supported limit for kernel this_is_kernel: {{.*}} bytes (max permitted PTSS 1024 bytes)

#define SIZE 16
#define NO_TAIL()

#define LEXP_0(eval_macro, tail_macro) tail_macro()
#define LEXP_1(eval_macro, tail_macro) \
  eval_macro(0) LEXP_0(eval_macro, tail_macro)
#define LEXP_2(eval_macro, tail_macro) \
  eval_macro(1) LEXP_1(eval_macro, tail_macro)
#define LEXP_3(eval_macro, tail_macro) \
  eval_macro(2) LEXP_2(eval_macro, tail_macro)
#define LEXP_4(eval_macro, tail_macro) \
  eval_macro(3) LEXP_3(eval_macro, tail_macro)
#define LEXP_5(eval_macro, tail_macro) \
  eval_macro(4) LEXP_4(eval_macro, tail_macro)
#define LEXP_6(eval_macro, tail_macro) \
  eval_macro(5) LEXP_5(eval_macro, tail_macro)
#define LEXP_7(eval_macro, tail_macro) \
  eval_macro(6) LEXP_6(eval_macro, tail_macro)
#define LEXP_8(eval_macro, tail_macro) \
  eval_macro(7) LEXP_7(eval_macro, tail_macro)
#define LEXP_9(eval_macro, tail_macro) \
  eval_macro(8) LEXP_8(eval_macro, tail_macro)
#define LEXP_10(eval_macro, tail_macro) \
  eval_macro(9) LEXP_9(eval_macro, tail_macro)
#define LEXP_11(eval_macro, tail_macro) \
  eval_macro(10) LEXP_10(eval_macro, tail_macro)
#define LEXP_12(eval_macro, tail_macro) \
  eval_macro(11) LEXP_11(eval_macro, tail_macro)
#define LEXP_13(eval_macro, tail_macro) \
  eval_macro(12) LEXP_12(eval_macro, tail_macro)
#define LEXP_14(eval_macro, tail_macro) \
  eval_macro(13) LEXP_13(eval_macro, tail_macro)
#define LEXP_15(eval_macro, tail_macro) \
  eval_macro(14) LEXP_14(eval_macro, tail_macro)
#define LEXP_16(eval_macro, tail_macro) \
  eval_macro(15) LEXP_15(eval_macro, tail_macro)


#define LEXP(n, eval_macro, tail_macro) n(eval_macro, tail_macro)
#define _STR(x) LEXP_##x
#define STR(x) _STR(x)

void this_is_function_with_spills(global const int *in, global int *out,
                       global const uint *offset) {
#define read_input(idx) int _data##idx = in[get_global_id(0) + offset[idx]];
#define sum_nonaff_data(idx) _data##idx *idx +
#define sum_nonaff_data_tail() 0

#define sum_sq_data(idx) _data##idx *_data##idx +
#define sum_sq_data_tail() 0

  LEXP(STR(SIZE), read_input, NO_TAIL)
  out[get_global_id(0) * 2] =
      LEXP(STR(SIZE), sum_nonaff_data, sum_nonaff_data_tail);
  out[get_global_id(0) * 2 + 1] =
      LEXP(STR(SIZE), sum_sq_data, sum_sq_data_tail);
}

__attribute__((intel_reqd_sub_group_size(32)))
kernel void this_is_kernel(global const int *in, global int *out,
                       global const uint *offset) {
    this_is_function_with_spills(in, out, offset);
}
