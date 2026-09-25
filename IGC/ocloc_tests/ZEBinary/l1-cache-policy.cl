/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: oneapi-readelf

// RUN: %if pvc-supported %{ ocloc compile -file %s -device pvc -o %t.pvc %}
// RUN: %if pvc-supported %{ oneapi-readelf -p .ze_info %t.pvc | FileCheck %s --check-prefix=WBP %}
// RUN: %if bmg-supported %{ ocloc compile -file %s -device bmg -o %t.bmg %}
// RUN: %if bmg-supported %{ oneapi-readelf -p .ze_info %t.bmg | FileCheck %s --check-prefix=WBP %}
// RUN: %if dg2-supported %{ ocloc compile -file %s -device dg2 -o %t.dg2 %}
// RUN: %if dg2-supported %{ oneapi-readelf -p .ze_info %t.dg2 | FileCheck %s --check-prefix=WB %}
// RUN: %if cri-supported %{ ocloc compile -file %s -device cri -o %t.cri %}
// RUN: %if cri-supported %{ oneapi-readelf -p .ze_info %t.cri | FileCheck %s --check-prefix=WB %}

// WBP: l1_cache_policy: wbp
// WB:  l1_cache_policy: wb

kernel void test(global int *out) { out[0] = 0; }
