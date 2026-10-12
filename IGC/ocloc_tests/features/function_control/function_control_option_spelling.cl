/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Checks that the FunctionControl internal option is accepted under the name its
// own definition documents: -cl-intel-functionControl / -ze-intel-functionControl.
//
// Background:
//   IGCInternalOptions.td defines the option as
//       // -cl-intel-functonControl [<n>] -ze-intel-functionControl [<n>]
//       defm functonControl : CommonSeparate<"functonControl">;
//   CommonSeparate derives every accepted spelling (-cl-, -cl-intel-, -ze-,
//   -ze-intel-, -ze-opt- and bare) from that one base string, so the typo in the
//   base string is carried by all of them and the spelling promised by the comment
//   exists nowhere. llvm::opt drops an unrecognized internal option without a
//   diagnostic, so -ze-intel-functionControl was silently ignored.
//
// The option is observed through its effect rather than through parsing alone:
// FunctionControl=3 is FLAG_FCALL_FORCE_STACKCALL, so ProcessFuncAttributes marks
// every non-kernel function noinline + visaStackCall and PrivateMemoryResolution
// then emits the "Stack call has been detected" warning. FunctionControl=1 is
// FLAG_FCALL_FORCE_INLINE and, like the default, leaves no stack call behind.
// Only -internal_options is used, so the test does not depend on regkeys.

// REQUIRES: dg2-supported

// Control: with no option at all the callee is inlined, so there is no stack call.
// RUN: ocloc compile -file %s -device dg2 2>&1 | FileCheck %s --check-prefix=CHECK-INLINED

// The long-standing misspelled spelling has to keep working.
// RUN: ocloc compile -file %s -device dg2 -internal_options "-cl-intel-functonControl 3" 2>&1 | FileCheck %s --check-prefix=CHECK-STACKCALL
// RUN: ocloc compile -file %s -device dg2 -internal_options "-ze-intel-functonControl 3" 2>&1 | FileCheck %s --check-prefix=CHECK-STACKCALL

// The documented spelling has to work as well.
// RUN: ocloc compile -file %s -device dg2 -internal_options "-cl-intel-functionControl 3" 2>&1 | FileCheck %s --check-prefix=CHECK-STACKCALL
// RUN: ocloc compile -file %s -device dg2 -internal_options "-ze-intel-functionControl 3" 2>&1 | FileCheck %s --check-prefix=CHECK-STACKCALL

// ... and the value has to be carried through, not just the option name: 1 is
// FLAG_FCALL_FORCE_INLINE and must not produce a stack call.
// RUN: ocloc compile -file %s -device dg2 -internal_options "-ze-intel-functionControl 1" 2>&1 | FileCheck %s --check-prefix=CHECK-INLINED

// CHECK-INLINED-NOT: Stack call has been detected
// CHECK-INLINED: Build succeeded.

// CHECK-STACKCALL: warning: in kernel 'fctl': Stack call has been detected
// CHECK-STACKCALL: Build succeeded.

int helper(int a, int b) {
  int r = 0;
  for (int i = 0; i < 8; i++)
    r += (a ^ (b + i)) * (i + 1);
  return r;
}

__kernel void fctl(__global int *in, __global int *out) {
  int i = get_global_id(0);
  out[i] = helper(in[i], in[i + 1]);
}
