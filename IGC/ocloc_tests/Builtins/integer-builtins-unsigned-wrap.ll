;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Valid inputs make these builtins wrap as unsigned arithmetic, so the result
; can take either sign and each sign test below must stay a runtime compare.

; REQUIRES: regkeys, bmg-supported

; RUN: llvm-as %OPAQUE_PTR_FLAG% < %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options "-igc_opts 'PrintToConsole=1,PrintBefore=EmitPass'" 2>&1 | FileCheck %s

; mul_hi(0xffff, 0xffff) is 0xfffe, so the result can reach 0x8000.
define spir_kernel void @mul_hi_ushort(i16 %x, i16 %y, ptr addrspace(1) %out) {
; CHECK-LABEL: define spir_kernel void @mul_hi_ushort(
; CHECK: store i32 %
  %hi = call spir_func i16 @_Z20__spirv_ocl_u_mul_hitt(i16 %x, i16 %y)
  %hi.ext = zext i16 %hi to i32
  %top.bit = icmp uge i32 %hi.ext, 32768
  %res = zext i1 %top.bit to i32
  store i32 %res, ptr addrspace(1) %out
  ret void
}

; mul24 keeps the low 32 bits of a 46-bit product, which can be negative.
define spir_kernel void @mul24_int(i32 %x, i32 %y, ptr addrspace(1) %out) {
; CHECK-LABEL: define spir_kernel void @mul24_int(
; CHECK: store i32 %
  %x24 = and i32 %x, 8388607
  %y24 = and i32 %y, 8388607
  %prod = call spir_func i32 @_Z19__spirv_ocl_s_mul24ii(i32 %x24, i32 %y24)
  %neg = icmp slt i32 %prod, 0
  %res = zext i1 %neg to i32
  store i32 %res, ptr addrspace(1) %out
  ret void
}

; mad24(1, 1, INT_MAX) wraps to INT_MIN as unsigned arithmetic.
define spir_kernel void @mad24_int(i32 %z, ptr addrspace(1) %out) {
; CHECK-LABEL: define spir_kernel void @mad24_int(
; CHECK: store i32 %
  %z.nonneg = and i32 %z, 2147483647
  %sum = call spir_func i32 @_Z19__spirv_ocl_s_mad24iii(i32 1, i32 1, i32 %z.nonneg)
  %neg = icmp slt i32 %sum, 0
  %res = zext i1 %neg to i32
  store i32 %res, ptr addrspace(1) %out
  ret void
}

; mad_hi(x, y, INT_MAX) wraps negative as unsigned for any positive mul_hi.
define spir_kernel void @mad_hi_int(i32 %x, i32 %y, ptr addrspace(1) %out) {
; CHECK-LABEL: define spir_kernel void @mad_hi_int(
; CHECK: store i32 %
  %sum = call spir_func i32 @_Z20__spirv_ocl_s_mad_hiiii(i32 %x, i32 %y, i32 2147483647)
  %neg = icmp slt i32 %sum, 0
  %res = zext i1 %neg to i32
  store i32 %res, ptr addrspace(1) %out
  ret void
}

declare spir_func i16 @_Z20__spirv_ocl_u_mul_hitt(i16, i16)
declare spir_func i32 @_Z19__spirv_ocl_s_mul24ii(i32, i32)
declare spir_func i32 @_Z19__spirv_ocl_s_mad24iii(i32, i32, i32)
declare spir_func i32 @_Z20__spirv_ocl_s_mad_hiiii(i32, i32, i32)
