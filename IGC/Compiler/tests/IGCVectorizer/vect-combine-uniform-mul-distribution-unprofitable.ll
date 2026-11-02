;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vect-combine -dce -verify < %s | FileCheck %s

; Distribution only pays off when it removes a non uniform instruction.
; Below it removes nothing and only adds the uniform b * c mul.

; the fmul already folds into mad(sub, c, d)
; CHECK-LABEL: define spir_kernel void @product_feeds_fadd(
; CHECK-NOT: distributed
; CHECK: %sub = fsub fast <16 x float> %a, %b
; CHECK: %mul = fmul fast <16 x float> %sub, %c
; CHECK: %res = fadd fast <16 x float> %mul, %d
define spir_kernel void @product_feeds_fadd(<16 x float> %a0, <16 x float> %b, <16 x float> %c, <16 x float> %d, ptr addrspace(1) %p) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a = insertelement <16 x float> %a0, float %lane_f, i32 0
  %sub = fsub fast <16 x float> %a, %b
  %mul = fmul fast <16 x float> %sub, %c
  %res = fadd fast <16 x float> %mul, %d
  store <16 x float> %res, ptr addrspace(1) %p
  ret void
}

; the fmul folds into the accumulating mad, distributing would sink the
; loop invariant fsub into the loop
; CHECK-LABEL: define spir_kernel void @fsub_outside_loop(
; CHECK-NOT: distributed
; CHECK: entry:
; CHECK: %sub = fsub fast <16 x float> %a, %b
; CHECK: body:
; CHECK: %mul = fmul fast <16 x float> %sub, %c
define spir_kernel void @fsub_outside_loop(<16 x float> %a0, <16 x float> %b, <16 x float> %c, ptr addrspace(1) %p, i32 %n) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a = insertelement <16 x float> %a0, float %lane_f, i32 0
  %sub = fsub fast <16 x float> %a, %b
  br label %body

body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %body ]
  %acc = phi <16 x float> [ zeroinitializer, %entry ], [ %acc.next, %body ]
  %mul = fmul fast <16 x float> %sub, %c
  %acc.next = fadd fast <16 x float> %acc, %mul
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, %n
  br i1 %cmp, label %body, label %exit

exit:
  store <16 x float> %acc.next, ptr addrspace(1) %p
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()

attributes #0 = { convergent nounwind }

!igc.functions = !{!0, !4}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
!0 = !{ptr @product_feeds_fadd, !1}
!4 = !{ptr @fsub_outside_loop, !1}

!10 = !{!"requiredSubGroupSize", i32 16}
!13 = !{!"FuncMDMap[0]", ptr @product_feeds_fadd}
!14 = !{!"FuncMDValue[0]", !10}
!15 = !{!"FuncMDMap[1]", ptr @fsub_outside_loop}
!16 = !{!"FuncMDValue[1]", !10}
!35 = !{!"FuncMD", !13, !14, !15, !16}
!36 = !{!"ModuleMD", !35}
!IGCMetadata = !{!36}
