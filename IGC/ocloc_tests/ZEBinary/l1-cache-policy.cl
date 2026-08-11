/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: pvc-supported, bmg-supported, dg2-supported, cri-supported, oneapi-readelf

// RUN: ocloc compile -file %s -device pvc -o %t.pvc
// RUN: oneapi-readelf -p .ze_info %t.pvc | FileCheck %s --check-prefix=WBP
// RUN: ocloc compile -file %s -device bmg -o %t.bmg
// RUN: oneapi-readelf -p .ze_info %t.bmg | FileCheck %s --check-prefix=WBP
// RUN: ocloc compile -file %s -device dg2 -o %t.dg2
// RUN: oneapi-readelf -p .ze_info %t.dg2 | FileCheck %s --check-prefix=WB
// RUN: ocloc compile -file %s -device cri -o %t.cri
// RUN: oneapi-readelf -p .ze_info %t.cri | FileCheck %s --check-prefix=WB

// WBP: l1_cache_policy: wbp
// WB:  l1_cache_policy: wb

kernel void test(global int *out) { out[0] = 0; }
