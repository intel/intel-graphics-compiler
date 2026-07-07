/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if has_dpas property is set correctly in ZEInfo when calling
// dpas instruction in inline assembly.
//


// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1'" \
// RUN:     -device dg2 | FileCheck %s

// CHECK:     has_dpas:        true

void kernel test() {
    __asm__ volatile("{\n"
            ".decl DUMMY_DPAS_SRC v_type=G type=ud num_elts=128\n"
            ".decl DUMMY_DPAS_DST v_type=G type=f num_elts=128\n"
            "dpas.bf.bf.8.1 (M1,8) DUMMY_DPAS_DST.0 V0.0 DUMMY_DPAS_SRC.0 DUMMY_DPAS_SRC(0,0)\n"
   "}\n");
}
