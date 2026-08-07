;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, debug, regkeys, llvm-16-plus
; RUN: split-file %s %t
; RUN: llvm-as %t/type.ll -o %t/type.bc
; RUN: not ocloc compile -llvm_input -file %t/type.bc -device cri -options "-igc_opts 'EnableOpaquePointersBackend=1,EnableFP4Dpas=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-ERR
; RUN: llvm-as %t/precision.ll -o %t/precision.bc
; RUN: not ocloc compile -llvm_input -file %t/precision.bc -device cri -options "-igc_opts 'EnableOpaquePointersBackend=1,EnableFP4Dpas=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-ERR

; Each split module changes one part of the packed intrinsic contract.
; A malformed physical-layout request must terminate compilation.

; CHECK-ERR: internal compiler error
;--- type.ll
target datalayout = "e-p:64:64:64-i8:8:8-i16:16:16-i32:32:32-i64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"
define spir_kernel void @invalid_type(ptr addrspace(1) %ap, ptr addrspace(1) %bp, ptr addrspace(1) %sap, ptr addrspace(1) %sbp, ptr addrspace(1) %out) #0 !intel_reqd_sub_group_size !0 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a = load <8 x i16>, ptr addrspace(1) %ap, align 16
  %b = load <8 x i32>, ptr addrspace(1) %bp, align 32
  %sagep = getelementptr <2 x i8>, ptr addrspace(1) %sap, i64 %lane
  %sbgep = getelementptr <2 x i8>, ptr addrspace(1) %sbp, i64 %lane
  %sa = load <2 x i8>, ptr addrspace(1) %sagep, align 2
  %sb = load <2 x i8>, ptr addrspace(1) %sbgep, align 2
  %r = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %sa, <2 x i8> %sb, i32 13, i32 13,
      i32 0, i32 0)
  store <8 x float> %r, ptr addrspace(1) %out, align 32
  ret void
}
declare i16 @llvm.genx.GenISA.simdLaneId()
declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
    <8 x float>, <8 x i16>, <8 x i32>, <2 x i8>, <2 x i8>, i32, i32,
    i32, i32)
attributes #0 = { nounwind }
!0 = !{i32 16}

;--- precision.ll
target datalayout = "e-p:64:64:64-i8:8:8-i16:16:16-i32:32:32-i64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"
define spir_kernel void @invalid_precision(ptr addrspace(1) %ap, ptr addrspace(1) %bp, ptr addrspace(1) %sap, ptr addrspace(1) %sbp, ptr addrspace(1) %out) #0 !intel_reqd_sub_group_size !0 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a = load <8 x i16>, ptr addrspace(1) %ap, align 16
  %b = load <8 x i32>, ptr addrspace(1) %bp, align 32
  %sagep = getelementptr <4 x i8>, ptr addrspace(1) %sap, i64 %lane
  %sbgep = getelementptr <4 x i8>, ptr addrspace(1) %sbp, i64 %lane
  %sa = load <4 x i8>, ptr addrspace(1) %sagep, align 4
  %sb = load <4 x i8>, ptr addrspace(1) %sbgep, align 4
  %r = call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %sa, <4 x i8> %sb, i32 7, i32 13,
      i32 0, i32 0)
  store <8 x float> %r, ptr addrspace(1) %out, align 32
  ret void
}
declare i16 @llvm.genx.GenISA.simdLaneId()
declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
    <8 x float>, <8 x i16>, <8 x i32>, <4 x i8>, <4 x i8>, i32, i32,
    i32, i32)
attributes #0 = { nounwind }
!0 = !{i32 16}

