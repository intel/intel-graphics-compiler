;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, cri-supported, llvm-16-plus
; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device cri \
; RUN: -options "-igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintBefore=EmitPass'" \
; RUN: -out_dir /dev/null 2>&1 | FileCheck %s

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64"

; Compare bits to distinguish +0 from -0. For nonzero inputs stepping to zero,
; the result keeps the input sign; equal signed-zero inputs return the target.
; CHECK-LABEL: define {{.*}}spir_kernel void @test_bfloat(
; CHECK: store i32 1,
define spir_kernel void @test_bfloat(ptr addrspace(1) %out) {
  %negative = call spir_func bfloat @_Z21__spirv_ocl_nextafterDF16bDF16b(bfloat 0xR8001, bfloat 0xR3F80)
  %positive = call spir_func bfloat @_Z21__spirv_ocl_nextafterDF16bDF16b(bfloat 0xR0001, bfloat 0xRBF80)
  %equal_negative = call spir_func bfloat @_Z21__spirv_ocl_nextafterDF16bDF16b(bfloat 0xR0000, bfloat 0xR8000)
  %equal_positive = call spir_func bfloat @_Z21__spirv_ocl_nextafterDF16bDF16b(bfloat 0xR8000, bfloat 0xR0000)
  %negative_bits = bitcast bfloat %negative to i16
  %positive_bits = bitcast bfloat %positive to i16
  %equal_negative_bits = bitcast bfloat %equal_negative to i16
  %equal_positive_bits = bitcast bfloat %equal_positive to i16
  %a = icmp eq i16 %negative_bits, -32768
  %b = icmp eq i16 %positive_bits, 0
  %c = icmp eq i16 %equal_negative_bits, -32768
  %d = icmp eq i16 %equal_positive_bits, 0
  %ab = and i1 %a, %b
  %cd = and i1 %c, %d
  %ok = and i1 %ab, %cd
  %result = zext i1 %ok to i32
  store i32 %result, ptr addrspace(1) %out
  ret void
}

; CHECK-LABEL: define {{.*}}spir_kernel void @test_bfloat_vector(
; CHECK: store i32 1,
define spir_kernel void @test_bfloat_vector(ptr addrspace(1) %out) {
  %value = call spir_func <4 x bfloat> @_Z21__spirv_ocl_nextafterDv4_DF16bS_(
      <4 x bfloat> <bfloat 0xR8001, bfloat 0xR0001, bfloat 0xR0000, bfloat 0xR8000>,
      <4 x bfloat> <bfloat 0xR3F80, bfloat 0xRBF80, bfloat 0xR8000, bfloat 0xR0000>)
  %bits = bitcast <4 x bfloat> %value to <4 x i16>
  %lanes = icmp eq <4 x i16> %bits, <i16 -32768, i16 0, i16 -32768, i16 0>
  %a = extractelement <4 x i1> %lanes, i32 0
  %b = extractelement <4 x i1> %lanes, i32 1
  %c = extractelement <4 x i1> %lanes, i32 2
  %d = extractelement <4 x i1> %lanes, i32 3
  %ab = and i1 %a, %b
  %cd = and i1 %c, %d
  %ok = and i1 %ab, %cd
  %result = zext i1 %ok to i32
  store i32 %result, ptr addrspace(1) %out
  ret void
}

declare spir_func bfloat @_Z21__spirv_ocl_nextafterDF16bDF16b(bfloat, bfloat)
declare spir_func <4 x bfloat> @_Z21__spirv_ocl_nextafterDv4_DF16bS_(<4 x bfloat>, <4 x bfloat>)
