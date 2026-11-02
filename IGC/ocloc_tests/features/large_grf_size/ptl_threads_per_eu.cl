/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// A requested number of HW threads per EU selects the largest GRF number of the
// VRT modes with that many threads. Every mode up to 96 GRF runs 10 threads, so
// 10 threads get 96 GRF.

// REQUIRES: regkeys, ptl-supported

// RUN: ocloc compile -file %s -device ptl \
// RUN: -options "-cl-intel-reqd-eu-thread-count 10 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-10 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device ptl \
// RUN: -options "-cl-intel-reqd-eu-thread-count 4 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-4 --implicit-check-not HWThreadNumberPerEU

// CHECK-10: //.kernel foo
// CHECK-10: //.thread_config {{[.]*}}numGRF=96{{[.]*}}

// CHECK-4: //.kernel foo
// CHECK-4: //.thread_config {{[.]*}}numGRF=256{{[.]*}}

// PTL has no 3-thread or 9-thread mode, so 3 threads get the next larger count,
// 4, and 9 threads get 10.

// RUN: ocloc compile -file %s -device ptl \
// RUN: -options "-cl-intel-reqd-eu-thread-count 3 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-3 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device ptl \
// RUN: -options "-cl-intel-reqd-eu-thread-count 9 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-9 --implicit-check-not HWThreadNumberPerEU

// CHECK-3: //.kernel foo
// CHECK-3: //.thread_config {{[.]*}}numGRF=256{{[.]*}}

// CHECK-9: //.kernel foo
// CHECK-9: //.thread_config {{[.]*}}numGRF=96{{[.]*}}

__kernel void foo(__global int *p) { p[get_global_id(0)] = 0; }
