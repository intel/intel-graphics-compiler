;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vector-coalescer -dce < %s 2>&1 | FileCheck %s

; duplicates the coalescer doesn't fold, on purpose: that's CSE's job,
; we fold only the common cases that are short to implement

; copy behind a non-candidate
; CHECK-LABEL: @fneg_chain
; CHECK: %coalesced_input = shufflevector <8 x float> %n1, <8 x float> %n2, <16 x i32>
; CHECK: call <16 x float> @llvm.exp2.v16f32(<16 x float> %coalesced_input)
define spir_kernel void @fneg_chain(ptr addrspace(1) %in, ptr addrspace(1) %out) #0 {
entry:
  %x = load <8 x float>, ptr addrspace(1) %in, align 32
  %n1 = fneg <8 x float> %x
  %n2 = fneg <8 x float> %x
  %e1 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %n1)
  %e2 = call <8 x float> @llvm.exp2.v8f32(<8 x float> %n2)
  %s = fadd <8 x float> %e1, %e2
  store <8 x float> %s, ptr addrspace(1) %out, align 32
  ret void
}

; identical insertelement chains
; CHECK-LABEL: @insert_chain
; CHECK: %coalesced_input = shufflevector <2 x float> %v1, <2 x float> %v2, <4 x i32>
; CHECK: call <4 x float> @llvm.exp2.v4f32(<4 x float> %coalesced_input)
define spir_kernel void @insert_chain(ptr addrspace(1) %out, float %a, float %b) #0 {
entry:
  %v1.0 = insertelement <2 x float> undef, float %a, i32 0
  %v1 = insertelement <2 x float> %v1.0, float %b, i32 1
  %v2.0 = insertelement <2 x float> undef, float %a, i32 0
  %v2 = insertelement <2 x float> %v2.0, float %b, i32 1
  %e1 = call <2 x float> @llvm.exp2.v2f32(<2 x float> %v1)
  %e2 = call <2 x float> @llvm.exp2.v2f32(<2 x float> %v2)
  %s = fadd <2 x float> %e1, %e2
  store <2 x float> %s, ptr addrspace(1) %out, align 8
  ret void
}

; commuted intrinsic
; CHECK-LABEL: @maxnum_commuted
; CHECK: %coalesced_input = shufflevector <8 x float> %x, <8 x float> %y, <16 x i32>
; CHECK: %coalesced_input1 = shufflevector <8 x float> %y, <8 x float> %x, <16 x i32>
; CHECK: call <16 x float> @llvm.maxnum.v16f32(<16 x float> %coalesced_input, <16 x float> %coalesced_input1)
define spir_kernel void @maxnum_commuted(ptr addrspace(1) %in, ptr addrspace(1) %out) #0 {
entry:
  %p1 = getelementptr <8 x float>, ptr addrspace(1) %in, i32 1
  %x = load <8 x float>, ptr addrspace(1) %in, align 32
  %y = load <8 x float>, ptr addrspace(1) %p1, align 32
  %m1 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %x, <8 x float> %y)
  %m2 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %y, <8 x float> %x)
  %s = fmul <8 x float> %m1, %m2
  store <8 x float> %s, ptr addrspace(1) %out, align 32
  ret void
}

declare <8 x float> @llvm.exp2.v8f32(<8 x float>)
declare <2 x float> @llvm.exp2.v2f32(<2 x float>)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>)

attributes #0 = { convergent nounwind }

!igc.functions = !{!0, !10, !20}

!0 = !{ptr @fneg_chain, !1}
!10 = !{ptr @insert_chain, !1}
!20 = !{ptr @maxnum_commuted, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{!"requiredSubGroupSize", i32 16}
!4 = !{!"FuncMDValue[0]", !3}
!5 = !{!"FuncMDMap[0]", ptr @fneg_chain}
!14 = !{!"FuncMDValue[1]", !3}
!15 = !{!"FuncMDMap[1]", ptr @insert_chain}
!24 = !{!"FuncMDValue[2]", !3}
!25 = !{!"FuncMDMap[2]", ptr @maxnum_commuted}
!6 = !{!"FuncMD", !5, !4, !15, !14, !25, !24}
!7 = !{!"ModuleMD", !6}
!IGCMetadata = !{!7}
