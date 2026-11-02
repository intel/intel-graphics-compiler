;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers -licm --igc-vect-combine -dce -verify < %s | FileCheck %s

; Relies on LICM running before the vectorizer (OptimizeIR in ShaderCodeGen.cpp):
; it hoists the loop invariant fsub out of the loop, so the same block check
; bails. A loop invariant fsub left in the loop would be distributed, and LICM
; after IGCVectCombine could no longer hoist it.

; CHECK-LABEL: define spir_kernel void @loop_invariant_fsub(
; CHECK-NOT: distributed
; CHECK: entry:
; CHECK: %sub = fsub fast <16 x float> %a, %b
; CHECK: body:
; CHECK: %mul = fmul fast <16 x float> %sub, %c
define spir_kernel void @loop_invariant_fsub(<16 x float> %a0, <16 x float> %b, ptr addrspace(1) %cp, ptr addrspace(1) %p, i32 %n) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane_f = uitofp i16 %lane to float
  %a = insertelement <16 x float> %a0, float %lane_f, i32 0
  br label %body

body:
  %i = phi i32 [ 0, %entry ], [ %i.next, %body ]
  %acc = phi <16 x float> [ zeroinitializer, %entry ], [ %acc.next, %body ]
  %cptr = getelementptr <16 x float>, ptr addrspace(1) %cp, i32 %i
  %c = load <16 x float>, ptr addrspace(1) %cptr
  %sub = fsub fast <16 x float> %a, %b
  %mul = fmul fast <16 x float> %sub, %c
  %acc.next = call <16 x float> @llvm.maxnum.v16f32(<16 x float> %acc, <16 x float> %mul)
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, %n
  br i1 %cmp, label %body, label %exit

exit:
  store <16 x float> %acc.next, ptr addrspace(1) %p
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <16 x float> @llvm.maxnum.v16f32(<16 x float>, <16 x float>)

attributes #0 = { convergent nounwind }

!igc.functions = !{!0}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
!0 = !{ptr @loop_invariant_fsub, !1}

!10 = !{!"requiredSubGroupSize", i32 16}
!13 = !{!"FuncMDMap[0]", ptr @loop_invariant_fsub}
!14 = !{!"FuncMDValue[0]", !10}
!35 = !{!"FuncMD", !13, !14}
!36 = !{!"ModuleMD", !35}
!IGCMetadata = !{!36}
