/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if all functions that we defined for FP4/INT4 conversions
// compile successfully. If some functions would be not defined, we would see "faddr"
// instructions in the output that is present for indirect calls.

// UNSUPPORTED: system-windows
// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1,EnableEfficient64b=1'" -device cri | FileCheck %s --check-prefix=CHECK-VISA
// CHECK-VISA-NOT: faddr

__attribute__((intel_reqd_sub_group_size(16)))
kernel void
test_all(
    global uchar *out_uchar,
    global uchar2 *out_uchar2,
    global uchar3 *out_uchar3,
    global uchar4 *out_uchar4,
    global uchar8 *out_uchar8,
    global uchar16 *out_uchar16,

    global ushort *out_ushort,
    global ushort2 *out_ushort2,
    global ushort3 *out_ushort3,
    global ushort4 *out_ushort4,
    global ushort8 *out_ushort8,
    global ushort16 *out_ushort16,

    global half *out_half,
    global half2 *out_half2,
    global half3 *out_half3,
    global half4 *out_half4,
    global half8 *out_half8,
    global half16 *out_half16,

    global char *source_char,
    global char2 *source_char2,
    global char3 *source_char3,
    global char4 *source_char4,
    global char8 *source_char8,
    global char16 *source_char16,

    global uchar *source_uchar,
    global uchar2 *source_uchar2,
    global uchar3 *source_uchar3,
    global uchar4 *source_uchar4,
    global uchar8 *source_uchar8,
    global uchar16 *source_uchar16)
{
  // Int4 -> Bfloat8:
  out_uchar[0] = intel_convert_as_i4_bfloat8_as_uchar(source_char[0]);
  out_uchar2[0] = intel_convert_as_i42_bfloat82_as_uchar2(source_char2[0]);
  out_uchar3[0] = intel_convert_as_i43_bfloat83_as_uchar3(source_char3[0]);
  out_uchar4[0] = intel_convert_as_i44_bfloat84_as_uchar4(source_char4[0]);
  out_uchar8[0] = intel_convert_as_i48_bfloat88_as_uchar8(source_char8[0]);
  out_uchar16[0] = intel_convert_as_i416_bfloat816_as_uchar16(source_char16[0]);

  // Int4 -> Hfloat8:
  out_uchar[1] = intel_convert_as_i4_hfloat8_as_uchar(source_char[1]);
  out_uchar2[1] = intel_convert_as_i42_hfloat82_as_uchar2(source_char2[1]);
  out_uchar3[1] = intel_convert_as_i43_hfloat83_as_uchar3(source_char3[1]);
  out_uchar4[1] = intel_convert_as_i44_hfloat84_as_uchar4(source_char4[1]);
  out_uchar8[1] = intel_convert_as_i48_hfloat88_as_uchar8(source_char8[1]);
  out_uchar16[1] = intel_convert_as_i416_hfloat816_as_uchar16(source_char16[1]);

  // Int4 -> Bfloat16:
  out_ushort[0] = intel_convert_as_i4_bfloat16_as_ushort(source_char[2]);
  out_ushort2[0] = intel_convert_as_i42_bfloat162_as_ushort2(source_char2[2]);
  out_ushort3[0] = intel_convert_as_i43_bfloat163_as_ushort3(source_char3[2]);
  out_ushort4[0] = intel_convert_as_i44_bfloat164_as_ushort4(source_char4[2]);
  out_ushort8[0] = intel_convert_as_i48_bfloat168_as_ushort8(source_char8[2]);
  out_ushort16[0] = intel_convert_as_i416_bfloat1616_as_ushort16(source_char16[2]);

  // Int4 -> Half:
  out_half[0] = intel_convert_as_i4_half(source_char[3]);
  out_half2[0] = intel_convert_as_i42_half2(source_char2[3]);
  out_half3[0] = intel_convert_as_i43_half3(source_char3[3]);
  out_half4[0] = intel_convert_as_i44_half4(source_char4[3]);
  out_half8[0] = intel_convert_as_i48_half8(source_char8[3]);
  out_half16[0] = intel_convert_as_i416_half16(source_char16[3]);

  // e2m1

  // e2m1 -> Bfloat8:
  out_uchar[4] = intel_convert_as_e2m1_bfloat8_as_uchar(source_uchar[0]);
  out_uchar2[4] = intel_convert_as_e2m12_bfloat82_as_uchar2(source_uchar2[0]);
  out_uchar3[4] = intel_convert_as_e2m13_bfloat83_as_uchar3(source_uchar3[0]);
  out_uchar4[4] = intel_convert_as_e2m14_bfloat84_as_uchar4(source_uchar4[0]);
  out_uchar8[4] = intel_convert_as_e2m18_bfloat88_as_uchar8(source_uchar8[0]);
  out_uchar16[4] = intel_convert_as_e2m116_bfloat816_as_uchar16(source_uchar16[0]);

  // e2m1 -> Hfloat8:
  out_uchar[5] = intel_convert_as_e2m1_hfloat8_as_uchar(source_uchar[1]);
  out_uchar2[5] = intel_convert_as_e2m12_hfloat82_as_uchar2(source_uchar2[1]);
  out_uchar3[5] = intel_convert_as_e2m13_hfloat83_as_uchar3(source_uchar3[1]);
  out_uchar4[5] = intel_convert_as_e2m14_hfloat84_as_uchar4(source_uchar4[1]);
  out_uchar8[5] = intel_convert_as_e2m18_hfloat88_as_uchar8(source_uchar8[1]);
  out_uchar16[5] = intel_convert_as_e2m116_hfloat816_as_uchar16(source_uchar16[1]);

  // e2m1 -> Bfloat16:
  out_ushort[2] = intel_convert_as_e2m1_bfloat16_as_ushort(source_uchar[2]);
  out_ushort2[2] = intel_convert_as_e2m12_bfloat162_as_ushort2(source_uchar2[2]);
  out_ushort3[2] = intel_convert_as_e2m13_bfloat163_as_ushort3(source_uchar3[2]);
  out_ushort4[2] = intel_convert_as_e2m14_bfloat164_as_ushort4(source_uchar4[2]);
  out_ushort8[2] = intel_convert_as_e2m18_bfloat168_as_ushort8(source_uchar8[2]);
  out_ushort16[2] = intel_convert_as_e2m116_bfloat1616_as_ushort16(source_uchar16[2]);

  // e2m1 -> Half:
  out_half[2] = intel_convert_as_e2m1_half(source_uchar[3]);
  out_half2[2] = intel_convert_as_e2m12_half2(source_uchar2[3]);
  out_half3[2] = intel_convert_as_e2m13_half3(source_uchar3[3]);
  out_half4[2] = intel_convert_as_e2m14_half4(source_uchar4[3]);
  out_half8[2] = intel_convert_as_e2m18_half8(source_uchar8[3]);
  out_half16[2] = intel_convert_as_e2m116_half16(source_uchar16[3]);
}

__attribute__((intel_reqd_sub_group_size(32)))
kernel void
test_all_packed(
    global ushort *out_ushort,
    global ushort2 *out_ushort2,
    global ushort3 *out_ushort3,
    global ushort4 *out_ushort4,
    global ushort8 *out_ushort8,
    global ushort16 *out_ushort16,

    global uint *out_uint,
    global uint2 *out_uint2,
    global uint3 *out_uint3,
    global uint4 *out_uint4,
    global uint8 *out_uint8,
    global uint16 *out_uint16,

    global half *out_half,
    global half2 *out_half2,
    global half3 *out_half3,
    global half4 *out_half4,
    global half8 *out_half8,
    global half16 *out_half16,

    global char *source_char,
    global char2 *source_char2,
    global char3 *source_char3,
    global char4 *source_char4,
    global char8 *source_char8,
    global char16 *source_char16,

    global uchar *source_uchar,
    global uchar2 *source_uchar2,
    global uchar3 *source_uchar3,
    global uchar4 *source_uchar4,
    global uchar8 *source_uchar8,
    global uchar16 *source_uchar16)
{
  // int
  out_ushort[0] = intel_convert_as_i42_bfloat82_as_ushort_packed(source_char[0]);
  out_ushort2[0] = intel_convert_as_i44_bfloat84_as_ushort2_packed(source_char2[0]);
  out_ushort3[0] = intel_convert_as_i46_bfloat86_as_ushort3_packed(source_char3[0]);
  out_ushort4[0] = intel_convert_as_i48_bfloat88_as_ushort4_packed(source_char4[0]);
  out_ushort8[0] = intel_convert_as_i416_bfloat816_as_ushort8_packed(source_char8[0]);
  out_ushort16[0] = intel_convert_as_i432_bfloat832_as_ushort16_packed(source_char16[0]);

  out_ushort[1] = intel_convert_as_i42_hfloat82_as_ushort_packed(source_char[0]);
  out_ushort2[1] = intel_convert_as_i44_hfloat84_as_ushort2_packed(source_char2[0]);
  out_ushort3[1] = intel_convert_as_i46_hfloat86_as_ushort3_packed(source_char3[0]);
  out_ushort4[1] = intel_convert_as_i48_hfloat88_as_ushort4_packed(source_char4[0]);
  out_ushort8[1] = intel_convert_as_i416_hfloat816_as_ushort8_packed(source_char8[0]);
  out_ushort16[1] = intel_convert_as_i432_hfloat832_as_ushort16_packed(source_char16[0]);

  out_uint[0] = intel_convert_as_i42_bfloat162_as_uint_packed(source_char[0]);
  out_uint2[0] = intel_convert_as_i44_bfloat164_as_uint2_packed(source_char2[0]);
  out_uint3[0] = intel_convert_as_i46_bfloat166_as_uint3_packed(source_char3[0]);
  out_uint4[0] = intel_convert_as_i48_bfloat168_as_uint4_packed(source_char4[0]);
  out_uint8[0] = intel_convert_as_i416_bfloat1616_as_uint8_packed(source_char8[0]);
  out_uint16[0] = intel_convert_as_i432_bfloat1632_as_uint16_packed(source_char16[0]);

  out_uint[1] = intel_convert_as_i42_half2_as_uint_packed(source_char[0]);
  out_uint2[1] = intel_convert_as_i44_half4_as_uint2_packed(source_char2[0]);
  out_uint3[1] = intel_convert_as_i46_half6_as_uint3_packed(source_char3[0]);
  out_uint4[1] = intel_convert_as_i48_half8_as_uint4_packed(source_char4[0]);
  out_uint8[1] = intel_convert_as_i416_half16_as_uint8_packed(source_char8[0]);
  out_uint16[1] = intel_convert_as_i432_half32_as_uint16_packed(source_char16[0]);

  // e2m1
  out_ushort[4] = intel_convert_as_e2m12_bfloat82_as_ushort_packed(source_uchar[0]);
  out_ushort2[4] = intel_convert_as_e2m14_bfloat84_as_ushort2_packed(source_uchar2[0]);
  out_ushort3[4] = intel_convert_as_e2m16_bfloat86_as_ushort3_packed(source_uchar3[0]);
  out_ushort4[4] = intel_convert_as_e2m18_bfloat88_as_ushort4_packed(source_uchar4[0]);
  out_ushort8[4] = intel_convert_as_e2m116_bfloat816_as_ushort8_packed(source_uchar8[0]);
  out_ushort16[4] = intel_convert_as_e2m132_bfloat832_as_ushort16_packed(source_uchar16[0]);

  out_ushort[5] = intel_convert_as_e2m12_hfloat82_as_ushort_packed(source_uchar[0]);
  out_ushort2[5] = intel_convert_as_e2m14_hfloat84_as_ushort2_packed(source_uchar2[0]);
  out_ushort3[5] = intel_convert_as_e2m16_hfloat86_as_ushort3_packed(source_uchar3[0]);
  out_ushort4[5] = intel_convert_as_e2m18_hfloat88_as_ushort4_packed(source_uchar4[0]);
  out_ushort8[5] = intel_convert_as_e2m116_hfloat816_as_ushort8_packed(source_uchar8[0]);
  out_ushort16[5] = intel_convert_as_e2m132_hfloat832_as_ushort16_packed(source_uchar16[0]);

  out_uint[4] = intel_convert_as_e2m12_bfloat162_as_uint_packed(source_uchar[0]);
  out_uint2[4] = intel_convert_as_e2m14_bfloat164_as_uint2_packed(source_uchar2[0]);
  out_uint3[4] = intel_convert_as_e2m16_bfloat166_as_uint3_packed(source_uchar3[0]);
  out_uint4[4] = intel_convert_as_e2m18_bfloat168_as_uint4_packed(source_uchar4[0]);
  out_uint8[4] = intel_convert_as_e2m116_bfloat1616_as_uint8_packed(source_uchar8[0]);
  out_uint16[4] = intel_convert_as_e2m132_bfloat1632_as_uint16_packed(source_uchar16[0]);

  out_uint[5] = intel_convert_as_e2m12_half2_as_uint_packed(source_uchar[0]);
  out_uint2[5] = intel_convert_as_e2m14_half4_as_uint2_packed(source_uchar2[0]);
  out_uint3[5] = intel_convert_as_e2m16_half6_as_uint3_packed(source_uchar3[0]);
  out_uint4[5] = intel_convert_as_e2m18_half8_as_uint4_packed(source_uchar4[0]);
  out_uint8[5] = intel_convert_as_e2m116_half16_as_uint8_packed(source_uchar8[0]);
  out_uint16[5] = intel_convert_as_e2m132_half32_as_uint16_packed(source_uchar16[0]);
}
