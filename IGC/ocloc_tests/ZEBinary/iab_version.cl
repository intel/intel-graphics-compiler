/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This test checks if "iab_version" field is present in the .note.intelgt.compat
// section of the ZEBinary.

// UNSUPPORTED: sys32
// REQUIRES: pvc-supported, oneapi-readelf

// RUN: ocloc compile -file %s -device pvc -output %t -output_no_suffix
// RUN: oneapi-readelf %t.bin -SWn | FileCheck %s

// CHECK: Displaying notes found in: .note.intelgt.compat
// CHECK: IntelGT {{.*}} Unknown note type: (0x00000008) description data: {{[0-9]+}} 00 00 00

kernel void test(global int* out) {
    out[0] = 10;
}


