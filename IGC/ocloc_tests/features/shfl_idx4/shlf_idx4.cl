/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// UNSUPPORTED: system-windows
// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1,EnableEfficient64b=1'" -device cri | FileCheck %s --check-prefix=CHECK-VISA

/*
This test checks for generated shfl_idx4 instructions for given input size.
shfl_idx4 has a restrcition that the source with LUT has :ud type and src1 with data has :uw type with <2;1,0> regioning,
*/
// CHECK-VISA: .kernel "test_char1_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<0;16,1> [[SHFLSRC1:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=uw num_elts=64 align=wordx32
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_char1_simd32(global char* inchar, global uchar* outuchar)  {
  int gid = get_local_id(0);
  char source = inchar[gid];
  outuchar[gid] = intel_convert_as_i4_bfloat8_as_uchar(source);
}

// CHECK-VISA: .kernel "test_char2_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST_V2:.*]].0 [[SHFLSRC0_V2:.*]](0,0)<0;16,1> [[SHFLSRC1_V2:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0_V2]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1_V2]] v_type=G type=uw num_elts=64 align=wordx32
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_char2_simd32(global char2* inchar, global uchar2* outuchar) {
  int gid = get_local_id(0);
  char2 source = inchar[gid];
  outuchar[gid] = intel_convert_as_i42_bfloat82_as_uchar2(source);
}

// CHECK-VISA: .kernel "test_char4_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST_V4:.*]].0 [[SHFLSRC0_V4:.*]](0,0)<0;16,1> [[SHFLSRC1_V4:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0_V4]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1_V4]] v_type=G type=uw num_elts=64 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_char4_simd32(global char4* inchar, global uchar4* outuchar) {
  int gid = get_local_id(0);
  char4 source = inchar[gid];
  outuchar[gid] = intel_convert_as_i44_bfloat84_as_uchar4(source);
}

// CHECK-VISA: .kernel "test_char8_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST_V8:.*]].0 [[SHFLSRC0_V8:.*]](0,0)<0;16,1> [[SHFLSRC1_V8:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0_V8]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1_V8]] v_type=G type=uw num_elts=64 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_char8_simd32(global char8* inchar, global uchar8* outuchar) {
  int gid = get_local_id(0);
  char8 source = inchar[gid];
  outuchar[gid] = intel_convert_as_i48_bfloat88_as_uchar8(source);
}

// CHECK-VISA: .kernel "test_char16_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST_V16:.*]].0 [[SHFLSRC0_V16:.*]](0,0)<0;16,1> [[SHFLSRC1_V16:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0_V16]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1_V16]] v_type=G type=uw num_elts=64 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<2;1,0>
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_char16_simd32(global char16* inchar, global uchar16* outuchar) {
  int gid = get_local_id(0);
  char16 source = inchar[gid];
  outuchar[gid] = intel_convert_as_i416_bfloat816_as_uchar16(source);
}

// CHECK-VISA: .kernel "test_short1_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<0;16,1> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=128 align=wordx32
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_short1_simd32(global char* inchar, global ushort* out)  {
  int gid = get_local_id(0);
  char source = inchar[gid];
  out[gid] = intel_convert_as_i4_bfloat16_as_ushort(source);
}

// CHECK-VISA: .kernel "test_short2_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<0;16,1> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=128 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_short2_simd32(global char2* inchar, global ushort2* out)  {
  int gid = get_local_id(0);
  out[gid] = intel_convert_as_i42_bfloat162_as_ushort2(inchar[gid]);
}

// CHECK-VISA: .kernel "test_short4_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<0;16,1> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=128 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_short4_simd32(global char4* inchar, global ushort4* out)  {
  int gid = get_local_id(0);
  out[gid] = intel_convert_as_i44_bfloat164_as_ushort4(inchar[gid]);
}

// CHECK-VISA: .kernel "test_short8_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<0;16,1> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=128 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_short8_simd32(global char8* inchar, global ushort8* out)  {
  int gid = get_local_id(0);
  out[gid] = intel_convert_as_i48_bfloat168_as_ushort8(inchar[gid]);
}

// CHECK-VISA: .kernel "test_short16_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<0;16,1> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=128 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 32) {{.*}} {{.*}} {{.*}}<4;1,0>
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_short16_simd32(global char16* inchar, global ushort16* out)  {
  int gid = get_local_id(0);
  out[gid] = intel_convert_as_i416_bfloat1616_as_ushort16(inchar[gid]);
}

// CHECK-VISA: .kernel "test_e2m1_short1_packed_simd32"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 32) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<0;16,1> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=128 align=wordx32
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_e2m1_short1_packed_simd32(global uchar* inchar, global uint* out)  {
  int gid = get_local_id(0);
  char source = inchar[gid];
  out[gid] = intel_convert_as_e2m12_bfloat162_as_uint_packed(source);
}

// CHECK-VISA: .kernel "test_char1"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<1;1,0> [[SHFLSRC1:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=uw num_elts=32 align=wordx32
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_char1(global char* inchar, global uchar* outuchar)  {
  int gid = get_local_id(0);
  char source = inchar[gid];
  outuchar[gid] = intel_convert_as_i4_bfloat8_as_uchar(source);
}

// CHECK-VISA: .kernel "test_char2"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST_V2:.*]].0 [[SHFLSRC0_V2:.*]](0,0)<1;1,0> [[SHFLSRC1_V2:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0_V2]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1_V2]] v_type=G type=uw num_elts=32 align=wordx32
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_char2(global char2* inchar, global uchar2* outuchar) {
  int gid = get_local_id(0);
  char2 source = inchar[gid];
  outuchar[gid] = intel_convert_as_i42_bfloat82_as_uchar2(source);
}

// CHECK-VISA: .kernel "test_char4"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST_V4:.*]].0 [[SHFLSRC0_V4:.*]](0,0)<1;1,0> [[SHFLSRC1_V4:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0_V4]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1_V4]] v_type=G type=uw num_elts=32 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_char4(global char4* inchar, global uchar4* outuchar) {
  int gid = get_local_id(0);
  char4 source = inchar[gid];
  outuchar[gid] = intel_convert_as_i44_bfloat84_as_uchar4(source);
}

// CHECK-VISA: .kernel "test_char8"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST_V8:.*]].0 [[SHFLSRC0_V8:.*]](0,0)<1;1,0> [[SHFLSRC1_V8:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0_V8]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1_V8]] v_type=G type=uw num_elts=32 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_char8(global char8* inchar, global uchar8* outuchar) {
  int gid = get_local_id(0);
  char8 source = inchar[gid];
  outuchar[gid] = intel_convert_as_i48_bfloat88_as_uchar8(source);
}

// CHECK-VISA: .kernel "test_char16"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST_V16:.*]].0 [[SHFLSRC0_V16:.*]](0,0)<1;1,0> [[SHFLSRC1_V16:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0_V16]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1_V16]] v_type=G type=uw num_elts=32 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<2;1,0>
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_char16(global char16* inchar, global uchar16* outuchar) {
  int gid = get_local_id(0);
  char16 source = inchar[gid];
  outuchar[gid] = intel_convert_as_i416_bfloat816_as_uchar16(source);
}

/*
  int4/fp4 to fp16 have regioning restriction <4;1,1> on src1
*/
// CHECK-VISA: .kernel "test_short1"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<1;1,0> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=64 align=wordx32
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_short1(global char* inchar, global ushort* out)  {
  int gid = get_local_id(0);
  char source = inchar[gid];
  out[gid] = intel_convert_as_i4_bfloat16_as_ushort(source);
}

// CHECK-VISA: .kernel "test_short2"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<1;1,0> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=64 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_short2(global char2* inchar, global ushort2* out)  {
  int gid = get_local_id(0);
  out[gid] = intel_convert_as_i42_bfloat162_as_ushort2(inchar[gid]);
}

// CHECK-VISA: .kernel "test_short4"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<1;1,0> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=64 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_short4(global char4* inchar, global ushort4* out)  {
  int gid = get_local_id(0);
  out[gid] = intel_convert_as_i44_bfloat164_as_ushort4(inchar[gid]);
}

// CHECK-VISA: .kernel "test_short8"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<1;1,0> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=64 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_short8(global char8* inchar, global ushort8* out)  {
  int gid = get_local_id(0);
  out[gid] = intel_convert_as_i48_bfloat168_as_ushort8(inchar[gid]);
}

// CHECK-VISA: .kernel "test_short16"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<1;1,0> [[SHFLSRC1:.*]](0,0)<4;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=ub num_elts=64 align=wordx32
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
// CHECK-VISA: shfl_idx4 (M1_NM, 16) {{.*}} {{.*}} {{.*}}<4;1,0>
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_short16(global char16* inchar, global ushort16* out)  {
  int gid = get_local_id(0);
  out[gid] = intel_convert_as_i416_bfloat1616_as_ushort16(inchar[gid]);
}

// CHECK-VISA: .kernel "test_char1_packed"
// CHECK-VISA-DAG: shfl_idx4 (M1_NM, 16) [[SHFLDST:.*]].0 [[SHFLSRC0:.*]](0,0)<1;1,0> [[SHFLSRC1:.*]](0,0)<2;1,0>
// CHECK-VISA-DAG: .decl [[SHFLSRC0]] v_type=G type=ud num_elts=16 {{.*}}
// CHECK-VISA-DAG: .decl [[SHFLSRC1]] v_type=G type=uw num_elts=32 align=wordx32
__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_char1_packed(global char* inchar, global ushort* out)  {
  int gid = get_local_id(0);
  char source = inchar[gid];
  out[gid] = intel_convert_as_i42_bfloat82_as_ushort_packed(source);
}
