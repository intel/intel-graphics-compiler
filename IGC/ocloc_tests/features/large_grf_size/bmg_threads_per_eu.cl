/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Without VRT, a requested number of HW threads per EU selects one of the two GRF
// modes, 128 GRF with 8 threads or 256 GRF with 4 threads. The GRF number reaches
// vISA as TotalGRFNum instead of HWThreadNumberPerEU, so the IGC passes plan for
// the same number.

// REQUIRES: regkeys, bmg-supported

// RUN: ocloc compile -file %s -device bmg \
// RUN: -options "-cl-intel-reqd-eu-thread-count 4 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-4 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device bmg \
// RUN: -options "-cl-intel-reqd-eu-thread-count 8 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-8 --implicit-check-not HWThreadNumberPerEU

// CHECK-4: //.kernel foo
// CHECK-4: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
// CHECK-4: //.full_options {{.*}} -TotalGRFNum 256 {{.*}}

// CHECK-8: //.kernel foo
// CHECK-8: //.thread_config {{[.]*}}numGRF=128{{[.]*}}
// CHECK-8: //.full_options {{.*}} -TotalGRFNum 128 {{.*}}

// A thread count without a mode takes the next larger count, or the largest count
// if every mode has fewer threads: 2 threads get the 4-thread mode and 9 threads
// get the 8-thread mode.

// RUN: ocloc compile -file %s -device bmg \
// RUN: -options "-cl-intel-reqd-eu-thread-count 2 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-2 --implicit-check-not HWThreadNumberPerEU
// RUN: ocloc compile -file %s -device bmg \
// RUN: -options "-cl-intel-reqd-eu-thread-count 9 -igc_opts 'VISAOptions=-asmToConsole'" \
// RUN: | FileCheck %s --check-prefix=CHECK-9 --implicit-check-not HWThreadNumberPerEU

// CHECK-2: //.kernel foo
// CHECK-2: //.thread_config {{[.]*}}numGRF=256{{[.]*}}

// CHECK-9: //.kernel foo
// CHECK-9: //.thread_config {{[.]*}}numGRF=128{{[.]*}}

__kernel void foo(__global int *p) { p[get_global_id(0)] = 0; }
