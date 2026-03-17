/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Checks should be corrected for win builds
// UNSUPPORTED: system-windows

// Check if mad sequences are emitted as expected.

// RUN: %if bmg-supported %{ ocloc compile -file %s -options " -DTYPE=short -cl-fast-relaxed-math -igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1'" -device bmg 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-XE2-SHORT %}
// RUN: %if bmg-supported %{ ocloc compile -file %s -options " -DTYPE=float -cl-fast-relaxed-math -igc_opts 'DumpVISAASMToConsole=1'" -device bmg 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-XE2-FLOAT %}
// RUN: %if bmg-supported %{ ocloc compile -file %s -options " -DTYPE=double -cl-fast-relaxed-math -igc_opts 'DumpVISAASMToConsole=1'" -device bmg 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-XE2-DOUBLE %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -DTYPE=int -igc_opts 'DumpVISAASMToConsole=1'" -device cri 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-XE3P-INT %}

#define MAD(a, b, c) ((a) * (b) + (c))

// CHECK-LABEL: .kernel "test_add"
//
// CHECK-XE2-SHORT:  lsc_load.ugm (M1, 32) [[SRC_B_16:.*]]:d16u32
// CHECK-XE2-SHORT:  mov (M1, 32) [[SRC_B:.*]](0,0)<1> [[SRC_B_16_ALIAS:.*]](0,0)<2;1,0>
// CHECK-XE2-SHORT:  lsc_load.ugm (M1, 32) [[SRC_A_16:.*]]:d16u32
// CHECK-XE2-SHORT:  mov (M1, 32) [[SRC_A:.*]](0,0)<1> [[SRC_A_16_ALIAS:.*]](0,0)<2;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD1:.*]](0,0)<1> [[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD2:.*]](0,0)<1> [[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD3:.*]](0,0)<1> [[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD4:.*]](0,0)<1> [[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD5:.*]](0,0)<1> [[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD6:.*]](0,0)<1> [[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mov (M1, 32) [[DST:.*]](0,0)<1> [[MAD6_ALIAS:.*]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 [[DST:.*]]:d16u32
// CHECK-XE2-SHORT:  .decl [[SRC_B_16_ALIAS]] v_type=G type=w num_elts=64 align=wordx64 alias=<[[SRC_B_16]], 0>
// CHECK-XE2-SHORT:  .decl [[SRC_A_16_ALIAS]] v_type=G type=w num_elts=64 align=wordx64 alias=<[[SRC_A_16]], 0>
// CHECK-XE2-SHORT:  .decl [[MAD6_ALIAS]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[MAD6]], 0>
//
// CHECK-XE2-FLOAT:  lsc_load.ugm (M1, 32) [[SRC_B:.*]]:d32
// CHECK-XE2-FLOAT:  lsc_load.ugm (M1, 32) [[SRC_A:.*]]:d32
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD1:.*]](0,0)<1> [[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD2:.*]](0,0)<1> [[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD3:.*]](0,0)<1> [[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD4:.*]](0,0)<1> [[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD5:.*]](0,0)<1> [[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD6:.*]](0,0)<1> [[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 [[MAD6]]:d32
//
// CHECK-XE2-DOUBLE: lsc_load.ugm (M1, 32) [[SRC_B:.*]]:d64
// CHECK-XE2-DOUBLE: lsc_load.ugm (M1, 32) [[SRC_A:.*]]:d64
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD1:.*]](0,0)<1> [[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD2:.*]](0,0)<1> [[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD3:.*]](0,0)<1> [[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD4:.*]](0,0)<1> [[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD5:.*]](0,0)<1> [[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD6:.*]](0,0)<1> [[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 [[MAD6]]:d64
//
// CHECK-XE3P-INT:   lsc_load.ugm (M1, 32) [[SRC_B:.*]]:d32
// CHECK-XE3P-INT:   lsc_load.ugm (M1, 32) [[SRC_A:.*]]:d32
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD1:.*]](0,0)<1> [[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD2:.*]](0,0)<1> [[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD3:.*]](0,0)<1> [[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD4:.*]](0,0)<1> [[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD5:.*]](0,0)<1> [[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD6:.*]](0,0)<1> [[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0>
// CHECK-XE3P-INT:   lsc_store.ugm (M1, 32) flat({{.*}})[{{.*}}]:a32s [[MAD6]]:d32
kernel void test_add(global TYPE *a, global TYPE *b, global TYPE *c) {
  int i = get_global_id(0);
  TYPE mad1 = MAD(b[i], a[i], b[i]);
  TYPE mad2 = MAD(mad1, b[i], mad1);
  TYPE mad3 = MAD(mad2, mad1, mad2);
  TYPE mad4 = MAD(mad3, mad2, mad3);
  TYPE mad5 = MAD(mad4, mad3, mad4);
  c[i]      = MAD(mad5, mad4, mad5);
}

// CHECK-LABEL: .kernel "test_sub_1"
//
// CHECK-XE2-SHORT:  lsc_load.ugm (M1, 32) [[SRC_B_16:.*]]:d16u32
// CHECK-XE2-SHORT:  mov (M1, 32) [[SRC_B:.*]](0,0)<1> [[SRC_B_16_ALIAS:.*]](0,0)<2;1,0>
// CHECK-XE2-SHORT:  lsc_load.ugm (M1, 32) [[SRC_A_16:.*]]:d16u32
// CHECK-XE2-SHORT:  mov (M1, 32) [[SRC_A:.*]](0,0)<1> [[SRC_A_16_ALIAS:.*]](0,0)<2;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD1:.*]](0,0)<1> [[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> (-)[[SRC_B]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD2:.*]](0,0)<1> [[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> (-)[[MAD1]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD3:.*]](0,0)<1> [[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> (-)[[MAD2]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD4:.*]](0,0)<1> [[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> (-)[[MAD3]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD5:.*]](0,0)<1> [[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> (-)[[MAD4]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD6:.*]](0,0)<1> [[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> (-)[[MAD5]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mov (M1, 32) [[DST:.*]](0,0)<1> [[MAD6_ALIAS:.*]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 [[DST:.*]]:d16u32
// CHECK-XE2-SHORT:  .decl [[SRC_B_16_ALIAS]] v_type=G type=w num_elts=64 align=wordx64 alias=<[[SRC_B_16]], 0>
// CHECK-XE2-SHORT:  .decl [[SRC_A_16_ALIAS]] v_type=G type=w num_elts=64 align=wordx64 alias=<[[SRC_A_16]], 0>
// CHECK-XE2-SHORT:  .decl [[MAD6_ALIAS]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[MAD6]], 0>
//
// CHECK-XE2-FLOAT:  lsc_load.ugm (M1, 32) [[SRC_B:.*]]:d32
// CHECK-XE2-FLOAT:  lsc_load.ugm (M1, 32) [[SRC_A:.*]]:d32
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD1:.*]](0,0)<1> [[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> (-)[[SRC_B]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD2:.*]](0,0)<1> [[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> (-)[[MAD1]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD3:.*]](0,0)<1> [[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> (-)[[MAD2]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD4:.*]](0,0)<1> [[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> (-)[[MAD3]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD5:.*]](0,0)<1> [[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> (-)[[MAD4]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD6:.*]](0,0)<1> [[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> (-)[[MAD5]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 [[MAD6]]:d32
//
// CHECK-XE2-DOUBLE: lsc_load.ugm (M1, 32) [[SRC_B:.*]]:d64
// CHECK-XE2-DOUBLE: lsc_load.ugm (M1, 32) [[SRC_A:.*]]:d64
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD1:.*]](0,0)<1> [[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> (-)[[SRC_B]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD2:.*]](0,0)<1> [[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> (-)[[MAD1]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD3:.*]](0,0)<1> [[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> (-)[[MAD2]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD4:.*]](0,0)<1> [[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> (-)[[MAD3]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD5:.*]](0,0)<1> [[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> (-)[[MAD4]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD6:.*]](0,0)<1> [[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> (-)[[MAD5]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 [[MAD6]]:d64
//
// CHECK-XE3P-INT:   lsc_load.ugm (M1, 32) [[SRC_B:.*]]:d32
// CHECK-XE3P-INT:   lsc_load.ugm (M1, 32) [[SRC_A:.*]]:d32
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD1:.*]](0,0)<1> [[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> (-)[[SRC_B]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD2:.*]](0,0)<1> [[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> (-)[[MAD1]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD3:.*]](0,0)<1> [[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> (-)[[MAD2]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD4:.*]](0,0)<1> [[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> (-)[[MAD3]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD5:.*]](0,0)<1> [[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> (-)[[MAD4]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD6:.*]](0,0)<1> [[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> (-)[[MAD5]](0,0)<1;1,0>
// CHECK-XE3P-INT:   lsc_store.ugm (M1, 32) flat({{.*}})[{{.*}}]:a32s [[MAD6]]:d32
kernel void test_sub_1(global TYPE *a, global TYPE *b, global TYPE *c) {
  int i = get_global_id(0);
  TYPE mad1 = MAD(b[i], a[i], -b[i]);
  TYPE mad2 = MAD(mad1, b[i], -mad1);
  TYPE mad3 = MAD(mad2, mad1, -mad2);
  TYPE mad4 = MAD(mad3, mad2, -mad3);
  TYPE mad5 = MAD(mad4, mad3, -mad4);
  c[i]      = MAD(mad5, mad4, -mad5);
}

// CHECK-LABEL: .kernel "test_sub_2"
//
// CHECK-XE2-SHORT:  lsc_load.ugm (M1, 32) [[SRC_B_16:.*]]:d16u32
// CHECK-XE2-SHORT:  mov (M1, 32) [[SRC_B:.*]](0,0)<1> [[SRC_B_16_ALIAS:.*]](0,0)<2;1,0>
// CHECK-XE2-SHORT:  lsc_load.ugm (M1, 32) [[SRC_A_16:.*]]:d16u32
// CHECK-XE2-SHORT:  mov (M1, 32) [[SRC_A:.*]](0,0)<1> [[SRC_A_16_ALIAS:.*]](0,0)<2;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD1:.*]](0,0)<1> (-)[[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD2:.*]](0,0)<1> (-)[[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD3:.*]](0,0)<1> (-)[[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD4:.*]](0,0)<1> (-)[[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD5:.*]](0,0)<1> (-)[[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mad (M1, 32) [[MAD6:.*]](0,0)<1> (-)[[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  mov (M1, 32) [[DST:.*]](0,0)<1> [[MAD6_ALIAS:.*]](0,0)<1;1,0>
// CHECK-XE2-SHORT:  lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 [[DST:.*]]:d16u32
// CHECK-XE2-SHORT:  .decl [[SRC_B_16_ALIAS]] v_type=G type=w num_elts=64 align=wordx64 alias=<[[SRC_B_16]], 0>
// CHECK-XE2-SHORT:  .decl [[SRC_A_16_ALIAS]] v_type=G type=w num_elts=64 align=wordx64 alias=<[[SRC_A_16]], 0>
// CHECK-XE2-SHORT:  .decl [[MAD6_ALIAS]] v_type=G type=uw num_elts=32 align=wordx32 alias=<[[MAD6]], 0>
//
// CHECK-XE2-FLOAT:  lsc_load.ugm (M1, 32) [[SRC_B:.*]]:d32
// CHECK-XE2-FLOAT:  lsc_load.ugm (M1, 32) [[SRC_A:.*]]:d32
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD1:.*]](0,0)<1> (-)[[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD2:.*]](0,0)<1> (-)[[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD3:.*]](0,0)<1> (-)[[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD4:.*]](0,0)<1> (-)[[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD5:.*]](0,0)<1> (-)[[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  mad (M1, 32) [[MAD6:.*]](0,0)<1> (-)[[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0>
// CHECK-XE2-FLOAT:  lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 [[MAD6]]:d32
//
// CHECK-XE2-DOUBLE: lsc_load.ugm (M1, 32) [[SRC_B:.*]]:d64
// CHECK-XE2-DOUBLE: lsc_load.ugm (M1, 32) [[SRC_A:.*]]:d64
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD1:.*]](0,0)<1> (-)[[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD2:.*]](0,0)<1> (-)[[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD3:.*]](0,0)<1> (-)[[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD4:.*]](0,0)<1> (-)[[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD5:.*]](0,0)<1> (-)[[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: mad (M1, 32) [[MAD6:.*]](0,0)<1> (-)[[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0>
// CHECK-XE2-DOUBLE: lsc_store.ugm (M1, 32) flat[{{.*}}]:a64 [[MAD6]]:d64
//
// CHECK-XE3P-INT:   lsc_load.ugm (M1, 32) [[SRC_B:.*]]:d32
// CHECK-XE3P-INT:   lsc_load.ugm (M1, 32) [[SRC_A:.*]]:d32
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD1:.*]](0,0)<1> (-)[[SRC_A]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0> [[SRC_B]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD2:.*]](0,0)<1> (-)[[SRC_B]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0> [[MAD1]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD3:.*]](0,0)<1> (-)[[MAD1]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0> [[MAD2]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD4:.*]](0,0)<1> (-)[[MAD2]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0> [[MAD3]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD5:.*]](0,0)<1> (-)[[MAD3]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0> [[MAD4]](0,0)<1;1,0>
// CHECK-XE3P-INT:   mad (M1, 32) [[MAD6:.*]](0,0)<1> (-)[[MAD4]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0> [[MAD5]](0,0)<1;1,0>
// CHECK-XE3P-INT:   lsc_store.ugm (M1, 32) flat({{.*}})[{{.*}}]:a32s [[MAD6]]:d32
kernel void test_sub_2(global TYPE *a, global TYPE *b, global TYPE *c) {
  int i = get_global_id(0);
  TYPE mad1 = MAD(-b[i], a[i], b[i]);
  TYPE mad2 = MAD(-mad1, b[i], mad1);
  TYPE mad3 = MAD(-mad2, mad1, mad2);
  TYPE mad4 = MAD(-mad3, mad2, mad3);
  TYPE mad5 = MAD(-mad4, mad3, mad4);
  c[i]      = MAD(-mad5, mad4, mad5);
}
