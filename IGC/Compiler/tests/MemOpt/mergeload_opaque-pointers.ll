;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt -opaque-pointers -S -platformbmg -igc-memopt %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @findCorners(ptr addrspace(1) %eigptr, <8 x i32> %r0, float %i15) {
; CHECK:  %0 = load <3 x float>, ptr addrspace(1) %add.ptr20, align 4
; CHECK:  %1 = extractelement <3 x float> %0, i32 0
; CHECK:  %2 = extractelement <3 x float> %0, i32 1
; CHECK:  %3 = extractelement <3 x float> %0, i32 2
; CHECK:  %i18 = call float @llvm.maxnum.f32(float %1, float %i15)
; CHECK:  %i20 = call float @llvm.maxnum.f32(float %2, float %i18)
; CHECK:  %i22 = call float @llvm.maxnum.f32(float %3, float %i20)
entry:
  %i4 = extractelement <8 x i32> %r0, i64 0
  %inc = add nsw i32 %i4, 1
  %mul8 = shl nsw i32 %inc, 2
  %idx.ext9 = sext i32 %mul8 to i64
  %mul18 = shl nsw i32 %i4, 2
  %idx.ext19 = sext i32 %mul18 to i64
  %add.ptr20 = getelementptr inbounds i8, ptr addrspace(1) %eigptr, i64 %idx.ext19
  %i17 = load float, ptr addrspace(1) %add.ptr20, align 4
  %i18 = call float @llvm.maxnum.f32(float %i17, float %i15)
  %add.ptr28 = getelementptr inbounds i8, ptr addrspace(1) %eigptr, i64 %idx.ext9
  %i19 = load float, ptr addrspace(1) %add.ptr28, align 4
  %i20 = call float @llvm.maxnum.f32(float %i19, float %i18)
  %mul34 = add i32 %mul18, 8
  %idx.ext35 = sext i32 %mul34 to i64
  %add.ptr36 = getelementptr inbounds i8, ptr addrspace(1) %eigptr, i64 %idx.ext35
  %i21 = load float, ptr addrspace(1) %add.ptr36, align 4
  %i22 = call float @llvm.maxnum.f32(float %i21, float %i20)
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.maxnum.f32(float, float) #0

; uselistorder directives
uselistorder ptr @llvm.maxnum.f32, { 2, 1, 0 }

attributes #0 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}

!0 = distinct !{ptr @findCorners, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"implicit_arg_desc"}
