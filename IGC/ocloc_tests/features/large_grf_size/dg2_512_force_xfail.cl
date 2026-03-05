/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if the compilation will fail if we pass a flag with forcing 512 GRF per thread on an unsupported platform.
// UNSUPPORTED: system-windows
// REQUIRES: regkeys, dg2-supported
// RUN: not ocloc compile -file %s -options "  -ze-opt-512-GRF-per-thread -igc_opts 'VISAOptions=-asmToConsole'" -device dg2 2>&1 | FileCheck %s --check-prefix=CHECK-OCLOC
// CHECK-OCLOC: 512-grf-per-thread option is not supported on this platform

__kernel void foo() {

}
