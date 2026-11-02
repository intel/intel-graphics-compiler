/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// NVL-S has the Xe3 render core, so a requested number of HW threads per EU
// selects a GRF number from the Xe3 VRT modes, as on PTL. Every mode up to 96 GRF
// runs 10 threads, so 10 threads get 96 GRF. No mode has 2 threads, so 2 threads
// get the next larger count, 4, and 256 GRF.

// REQUIRES: regkeys, nvl-s-supported

// RUN: ocloc compile -file %s -device nvl-s \
// RUN: -options "-cl-intel-reqd-eu-thread-count 10 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-10 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device nvl-s \
// RUN: -options "-cl-intel-reqd-eu-thread-count 2 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-2 --implicit-check-not HWThreadNumberPerEU

// CHECK-10: //.kernel foo
// CHECK-10: //.thread_config {{[.]*}}numGRF=96{{[.]*}}

// CHECK-2: //.kernel foo
// CHECK-2: //.thread_config {{[.]*}}numGRF=256{{[.]*}}

__kernel void foo(__global int *p) { p[get_global_id(0)] = 0; }
