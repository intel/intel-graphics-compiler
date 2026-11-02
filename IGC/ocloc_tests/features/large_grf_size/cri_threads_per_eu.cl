/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// A requested number of HW threads per EU selects the largest GRF number of the
// VRT modes with that many threads: 512 for 4 threads and 192 for 8 threads.
// The GRF number reaches vISA as TotalGRFNum instead of HWThreadNumberPerEU.

// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-reqd-eu-thread-count 4 -igc_opts 'EnableEfficient64b=1,VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-4 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device cri -internal_options "-cl-intel-num-thread-per-eu 8" \
// RUN: -options "-igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-8 --implicit-check-not HWThreadNumberPerEU

// CHECK-4: //.kernel foo
// CHECK-4: //.thread_config {{[.]*}}numGRF=512{{[.]*}}
// CHECK-4: //.kernel bar
// CHECK-4: //.thread_config {{[.]*}}numGRF=512{{[.]*}}

// CHECK-8: //.kernel foo
// CHECK-8: //.thread_config {{[.]*}}numGRF=192{{[.]*}}
// CHECK-8: //.kernel bar
// CHECK-8: //.thread_config {{[.]*}}numGRF=192{{[.]*}}

// A thread count without a mode takes the next larger count, or the largest count
// if every mode has fewer threads: 1 and 2 threads get the 4-thread mode, and 5
// and 10 threads get the 8-thread mode.

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-reqd-eu-thread-count 2 -igc_opts 'EnableEfficient64b=1,VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-2 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-reqd-eu-thread-count 5 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-5 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-reqd-eu-thread-count 1 -igc_opts 'EnableEfficient64b=1,VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-1 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-reqd-eu-thread-count 10 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-10 --implicit-check-not HWThreadNumberPerEU

// CHECK-2: //.kernel foo
// CHECK-2: //.thread_config {{[.]*}}numGRF=512{{[.]*}}
// CHECK-2: //.kernel bar
// CHECK-2: //.thread_config {{[.]*}}numGRF=512{{[.]*}}

// CHECK-5: //.kernel foo
// CHECK-5: //.thread_config {{[.]*}}numGRF=192{{[.]*}}
// CHECK-5: //.kernel bar
// CHECK-5: //.thread_config {{[.]*}}numGRF=192{{[.]*}}

// CHECK-1: //.kernel foo
// CHECK-1: //.thread_config {{[.]*}}numGRF=512{{[.]*}}
// CHECK-1: //.kernel bar
// CHECK-1: //.thread_config {{[.]*}}numGRF=512{{[.]*}}

// CHECK-10: //.kernel foo
// CHECK-10: //.thread_config {{[.]*}}numGRF=192{{[.]*}}
// CHECK-10: //.kernel bar
// CHECK-10: //.thread_config {{[.]*}}numGRF=192{{[.]*}}

// A GRF size requested for a kernel by name still takes precedence over the
// module thread count, and only the other kernels get the thread-count GRF number.

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-reqd-eu-thread-count 8 -ze-opt-large-grf-kernel foo -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-LARGE-GRF-PER-KERNEL

// CHECK-LARGE-GRF-PER-KERNEL: //.kernel foo
// CHECK-LARGE-GRF-PER-KERNEL: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
// CHECK-LARGE-GRF-PER-KERNEL: //.kernel bar
// CHECK-LARGE-GRF-PER-KERNEL: //.thread_config {{[.]*}}numGRF=192{{[.]*}}

// An explicit module GRF size also keeps its precedence over the thread count.

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-reqd-eu-thread-count 4 -ze-opt-large-register-file -igc_opts 'EnableEfficient64b=1,VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-LARGE-REGISTER-FILE

// CHECK-LARGE-REGISTER-FILE: //.kernel foo
// CHECK-LARGE-REGISTER-FILE: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
// CHECK-LARGE-REGISTER-FILE: //.kernel bar
// CHECK-LARGE-REGISTER-FILE: //.thread_config {{[.]*}}numGRF=256{{[.]*}}

// A 128/256/512-GRF option also takes precedence over the thread count, and
// only the GRF number reaches vISA.

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-intel-reqd-eu-thread-count 8 -cl-intel-256-GRF-per-thread -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-256-GRF-OPTION --implicit-check-not HWThreadNumberPerEU

// CHECK-256-GRF-OPTION: //.kernel foo
// CHECK-256-GRF-OPTION: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
// CHECK-256-GRF-OPTION: //.kernel bar
// CHECK-256-GRF-OPTION: //.thread_config {{[.]*}}numGRF=256{{[.]*}}

__kernel void foo(__global int *p) { p[get_global_id(0)] = 0; }
__kernel void bar(__global int *p) { p[get_global_id(0)] = 1; }
