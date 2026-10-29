;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -igc-emit-visa -platformbmg -simd-mode 16 -regkey DumpVISAASMToConsole,VectorAlias=0 | FileCheck %s
; RUN: igc_opt %s --opaque-pointers -igc-emit-visa -platformbmg -simd-mode 16 -regkey DumpVISAASMToConsole,VectorAlias=4 | FileCheck %s

; emitJointWaveBroadcast maps the JointWaveBroadcast result to a uniform alias
; of its source, starting at the first broadcast lane. An extractelement with
; a constant index K from that result must read source lane (first lane + K).
; Here lanes are 2..9 and K = 3, so the extract must read src(0,5).
;
; With VectorAlias, the extract must not be VRA-aliased to the result: the
; result's symbol is remapped after the VRA alias variables are created, so
; the alias would read the original variable, which is never written.

; CHECK-LABEL: .kernel "test"
; CHECK:       lsc_load.ugm (M1, 16) [[SRC:[A-Za-z0-9_]+]]:d32
; CHECK:       mov (M1, 16) [[E:[A-Za-z0-9_]+]](0,0)<1> [[SRC]](0,5)<0;1,0>
; CHECK:       lsc_store.ugm (M1, 16) flat[{{[A-Za-z0-9_]+}}]:a64 [[E]]:d32

define spir_kernel void @test() {
entry:
  %lid = call i16 @llvm.genx.GenISA.simdLaneId()
  %lid32 = zext i16 %lid to i64
  %inp = getelementptr float, ptr addrspace(1) null, i64 %lid32
  %src = load float, ptr addrspace(1) %inp, align 4
  %jwb = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9>, i32 0)
  %e = extractelement <8 x float> %jwb, i32 3
  %outp = getelementptr float, ptr addrspace(1) null, i64 %lid32
  store float %e, ptr addrspace(1) %outp, align 4
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float, <8 x i32>, i32)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!IGCMetadata = !{!3}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"requiredSubGroupSize", i32 16}
