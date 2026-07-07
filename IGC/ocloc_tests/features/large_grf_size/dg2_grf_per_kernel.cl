/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -ze-opt-large-grf-kernel foo -ze-opt-large-grf-kernel baz -igc_opts 'VISAOptions=-asmToConsole'" -device dg2 | FileCheck %s --check-prefix=CHECK-LARGE-GRF-PER-KERNEL
// RUN: ocloc compile -file %s -options " -ze-opt-large-register-file -ze-opt-regular-grf-kernel foo -ze-opt-regular-grf-kernel baz -igc_opts 'VISAOptions=-asmToConsole'" -device dg2 | FileCheck %s --check-prefix=CHECK-REGULAR-GRF-PER-KERNEL


// CHECK-LARGE-GRF-PER-KERNEL: //.kernel foo
// CHECK-LARGE-GRF-PER-KERNEL: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
// CHECK-LARGE-GRF-PER-KERNEL: //.kernel bar
// CHECK-LARGE-GRF-PER-KERNEL: //.thread_config {{[.]*}}numGRF=128{{[.]*}}
// CHECK-LARGE-GRF-PER-KERNEL: //.kernel baz
// CHECK-LARGE-GRF-PER-KERNEL: //.thread_config {{[.]*}}numGRF=256{{[.]*}}

// CHECK-REGULAR-GRF-PER-KERNEL: //.kernel foo
// CHECK-REGULAR-GRF-PER-KERNEL: //.thread_config {{[.]*}}numGRF=128{{[.]*}}
// CHECK-REGULAR-GRF-PER-KERNEL: //.kernel bar
// CHECK-REGULAR-GRF-PER-KERNEL: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
// CHECK-REGULAR-GRF-PER-KERNEL: //.kernel baz
// CHECK-REGULAR-GRF-PER-KERNEL: //.thread_config {{[.]*}}numGRF=128{{[.]*}}


__kernel void foo() {}
__kernel void bar() {}
__kernel void baz() {}

