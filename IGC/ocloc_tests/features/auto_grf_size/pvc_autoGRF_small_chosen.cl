/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, pvc-supported
// RUN: ocloc compile -file %s -options "  -cl-intel-enable-auto-large-GRF-mode -igc_opts 'VISAOptions=-asmToConsole'" -device pvc | FileCheck %s --check-prefix=CHECK-ASM

// CHECK-ASM: //.thread_config {{[.]*}}numGRF=128{{[.]*}}
// CHECK-ASM: //.full_options "{{.*}}-autoGRFSelection{{.*}}"
__kernel void foo() {

}