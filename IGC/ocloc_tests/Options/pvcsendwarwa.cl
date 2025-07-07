/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-cl-std=CL2.0 -igc_opts 'VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefix=CHECK-DEFAULT
// RUN: ocloc compile -file %s -device pvc -internal_options "-cl-intel-disable-sendwarwa" -options "-cl-std=CL2.0 -igc_opts 'VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefix=CHECK-DISABLE
// RUN: ocloc compile -file %s -device pvc -internal_options "-ze-opt-disable-sendwarwa" -options "-cl-std=CL2.0 -igc_opts 'VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefix=CHECK-DISABLE

// CHECK-DEFAULT: (W)     mov (1|M0)               null<1>:ud
// CHECK-DEFAULT: (W)     mov (1|M0)               null<1>:ud
// CHECK-DEFAULT: (W)     mov (1|M0)               null<1>:ud
// CHECK-DEFAULT:        sync.nop                             null
// CHECK-DEFAULT:        sync.allrd
// CHECK-DISABLE-NOT: (W)     mov (1|M0)               null<1>:ud
// CHECK-DISABLE-NOT:        sync.allrd

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__attribute__((reqd_work_group_size(32,1,1)))
__attribute__((intel_reqd_sub_group_size(16)))
kernel void gemm_kernel(global uint *A, global uint *B, global uint *C, long offset_A, long offset_B, long offset_C, uint lda, uint ldb, uint ldc, int m, int n, int k, int diag_C, double alpha_real, double beta_real, double alpha_imag, double beta_imag, uint flags) {
    global volatile int *____;
    (void) ____[get_local_id(0)];
    (void) ____[get_enqueued_local_size(0)];
    __asm__ volatile("" :: "rw.u"(A));
    __asm__ volatile("" :: "rw.u"(B));
    __asm__ volatile("" :: "rw.u"(C));
}

