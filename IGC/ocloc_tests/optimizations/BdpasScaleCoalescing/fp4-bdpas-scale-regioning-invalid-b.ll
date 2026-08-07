;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, debug, regkeys, llvm-16-plus
; RUN: llvm-as %s -o %t.bc
; RUN: not ocloc compile -llvm_input -file %t.bc -device cri -options "-igc_opts 'EnableOpaquePointersBackend=1,EnableFP4Dpas=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-ERR

; A packed FP4 intrinsic has an explicit B byte offset. It must be a multiple
; of sixteen. A malformed request must terminate compilation.

; CHECK-ERR: internal compiler error

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @invalid_packed_b(ptr addrspace(1) %a_ptr, ptr addrspace(1) %b_ptr, ptr addrspace(1) %sa_ptr, ptr addrspace(1) %sb_ptr, ptr addrspace(1) %out) #0 !intel_reqd_sub_group_size !0 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a = load <8 x i16>, ptr addrspace(1) %a_ptr, align 16
  %b = load <8 x i32>, ptr addrspace(1) %b_ptr, align 32
  %sa.gep = getelementptr <4 x i8>, ptr addrspace(1) %sa_ptr, i64 %lane
  %sb.gep = getelementptr <4 x i8>, ptr addrspace(1) %sb_ptr, i64 %lane
  %sa = load <4 x i8>, ptr addrspace(1) %sa.gep, align 4
  %sb = load <4 x i8>, ptr addrspace(1) %sb.gep, align 4
  %r = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %sa, <4 x i8> %sb, i32 13, i32 13,
      i32 0, i32 1)
  store <8 x float> %r, ptr addrspace(1) %out, align 32
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
    <8 x float>, <8 x i16>, <8 x i32>, <4 x i8>, <4 x i8>, i32, i32,
    i32, i32)
attributes #0 = { nounwind }
!0 = !{i32 16}
