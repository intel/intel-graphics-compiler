;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vect-combine -dce -verify < %s | FileCheck %s

; The fsub is hoisted before the loop, the fmul is in the loop since %c changes
; every iteration. Distributing would grow the loop body from N muls to
; 1 NoMask mul + N mads, so the fsub and fmul must be in the same block.

; CHECK-LABEL: define spir_kernel void @fsub_outside_loop_v16(
; CHECK-NOT: distributed
; CHECK: entry:
; CHECK: %sub = fsub fast <16 x float> %a, %b
; CHECK: body:
; CHECK: %mul = fmul fast <16 x float> %sub, %c
; CHECK: call <16 x float> @llvm.maxnum.v16f32(<16 x float> %acc, <16 x float> %mul)
define spir_kernel void @fsub_outside_loop_v16(<16 x float> %a0, <16 x float> %b, ptr addrspace(1) %cp, ptr addrspace(1) %p, i32 %n) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a = insertelement <16 x float> %a0, float %lane_f, i32 0
  %sub = fsub fast <16 x float> %a, %b
  br label %body

body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %body ]
  %acc = phi <16 x float> [ zeroinitializer, %entry ], [ %acc.next, %body ]
  %cptr = getelementptr <16 x float>, ptr addrspace(1) %cp, i32 %i
  %c = load <16 x float>, ptr addrspace(1) %cptr
  %mul = fmul fast <16 x float> %sub, %c
  %acc.next = call <16 x float> @llvm.maxnum.v16f32(<16 x float> %acc, <16 x float> %mul)
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, %n
  br i1 %cmp, label %body, label %exit

exit:
  store <16 x float> %acc.next, ptr addrspace(1) %p
  ret void
}

; same with a narrow vector
; CHECK-LABEL: define spir_kernel void @fsub_outside_loop_v2(
; CHECK-NOT: distributed
; CHECK: entry:
; CHECK: %sub = fsub fast <2 x float> %a, %b
; CHECK: body:
; CHECK: %mul = fmul fast <2 x float> %sub, %c
; CHECK: call <2 x float> @llvm.maxnum.v2f32(<2 x float> %acc, <2 x float> %mul)
define spir_kernel void @fsub_outside_loop_v2(<2 x float> %a0, <2 x float> %b, ptr addrspace(1) %cp, ptr addrspace(1) %p, i32 %n) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a = insertelement <2 x float> %a0, float %lane_f, i32 0
  %sub = fsub fast <2 x float> %a, %b
  br label %body

body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %body ]
  %acc = phi <2 x float> [ zeroinitializer, %entry ], [ %acc.next, %body ]
  %cptr = getelementptr <2 x float>, ptr addrspace(1) %cp, i32 %i
  %c = load <2 x float>, ptr addrspace(1) %cptr
  %mul = fmul fast <2 x float> %sub, %c
  %acc.next = call <2 x float> @llvm.maxnum.v2f32(<2 x float> %acc, <2 x float> %mul)
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, %n
  br i1 %cmp, label %body, label %exit

exit:
  store <2 x float> %acc.next, ptr addrspace(1) %p
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <16 x float> @llvm.maxnum.v16f32(<16 x float>, <16 x float>)
declare <2 x float> @llvm.maxnum.v2f32(<2 x float>, <2 x float>)

attributes #0 = { convergent nounwind }

!igc.functions = !{!0, !4}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
!0 = !{ptr @fsub_outside_loop_v16, !1}
!4 = !{ptr @fsub_outside_loop_v2, !1}

!10 = !{!"requiredSubGroupSize", i32 16}
!13 = !{!"FuncMDMap[0]", ptr @fsub_outside_loop_v16}
!14 = !{!"FuncMDValue[0]", !10}
!15 = !{!"FuncMDMap[1]", ptr @fsub_outside_loop_v2}
!16 = !{!"FuncMDValue[1]", !10}
!35 = !{!"FuncMD", !13, !14, !15, !16}
!36 = !{!"ModuleMD", !35}
!IGCMetadata = !{!36}
