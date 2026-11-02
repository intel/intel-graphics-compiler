/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// A requested number of HW threads per EU selects the largest GRF number of the
// VRT modes with that many threads. This device has efficient 64-bit addressing,
// so 2 threads get 512 GRF. It has no 320/448-GRF support, so automatic selection
// cannot use the 320-GRF mode, but a 3-thread request keeps it because it is the
// only mode with 3 threads.

// REQUIRES: regkeys, nvl-p-b0-supported

// RUN: ocloc compile -file %s -device nvl-p-b0 \
// RUN: -options "-cl-intel-reqd-eu-thread-count 2 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-2 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device nvl-p-b0 \
// RUN: -options "-cl-intel-reqd-eu-thread-count 3 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-3 --implicit-check-not HWThreadNumberPerEU

// CHECK-2: //.kernel foo
// CHECK-2: //.thread_config {{[.]*}}numGRF=512{{[.]*}}

// CHECK-3: //.kernel foo
// CHECK-3: //.thread_config {{[.]*}}numGRF=320{{[.]*}}

__kernel void foo(__global int *p) { p[get_global_id(0)] = 0; }
