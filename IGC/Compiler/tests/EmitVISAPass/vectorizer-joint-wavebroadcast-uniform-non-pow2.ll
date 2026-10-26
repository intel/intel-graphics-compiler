;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S --opaque-pointers -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s 2>&1 | FileCheck %s

; JointWaveBroadcast is uniform, so a binary op whose operands are the joint and
; a constant is uniform as well. With a non power of two vector width the uniform
; vector emission must not produce an instruction with execution size 0
; (lanesToSIMDMode(6) is not a valid SIMD mode).

; The 6 wide op is split into 4 + 2. Variable names are not matched since
; release builds drop LLVM value names.

; CHECK-LABEL: .kernel "fmul6"
; CHECK-NOT: (M1_NM, 0)
; CHECK: mul (M1_NM, 4) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[CST:[A-Za-z0-9_]+]](0,0)<1;1,0> [[SRC:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NEXT: mul (M1_NM, 2) [[DST]](0,4)<1> [[CST]](0,4)<1;1,0> [[SRC]](0,4)<1;1,0>
; CHECK-LABEL: .kernel "fadd6"
; CHECK-NOT: (M1_NM, 0)
; CHECK: add (M1_NM, 4) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[CST:[A-Za-z0-9_]+]](0,0)<1;1,0> [[SRC:[A-Za-z0-9_]+]](0,0)<1;1,0>
; CHECK-NEXT: add (M1_NM, 2) [[DST]](0,4)<1> [[CST]](0,4)<1;1,0> [[SRC]](0,4)<1;1,0>

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @fmul6(ptr addrspace(1) %out) {
entry:
  %lid = call i16 @llvm.genx.GenISA.simdLaneId()
  %lidf = uitofp i16 %lid to float
  %src = call float @llvm.exp2.f32(float %lidf)
  %vectorized_joint_wavebroadcast = call <6 x float> @llvm.genx.GenISA.JointWaveBroadcast.v6f32.f32.v6i32(float %src, <6 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5>, i32 0)
  %vectorized_binary = fmul <6 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, %vectorized_joint_wavebroadcast
  %lidz = zext i16 %lid to i64
  %p = getelementptr <6 x float>, ptr addrspace(1) %out, i64 %lidz
  store <6 x float> %vectorized_binary, ptr addrspace(1) %p, align 32
  ret void
}

define spir_kernel void @fadd6(ptr addrspace(1) %out) {
entry:
  %lid = call i16 @llvm.genx.GenISA.simdLaneId()
  %lidf = uitofp i16 %lid to float
  %src = call float @llvm.exp2.f32(float %lidf)
  %vectorized_joint_wavebroadcast = call <6 x float> @llvm.genx.GenISA.JointWaveBroadcast.v6f32.f32.v6i32(float %src, <6 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5>, i32 0)
  %vectorized_binary = fadd <6 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, %vectorized_joint_wavebroadcast
  %lidz = zext i16 %lid to i64
  %p = getelementptr <6 x float>, ptr addrspace(1) %out, i64 %lidz
  store <6 x float> %vectorized_binary, ptr addrspace(1) %p, align 32
  ret void
}

declare <6 x float> @llvm.genx.GenISA.JointWaveBroadcast.v6f32.f32.v6i32(float, <6 x i32>, i32)
declare float @llvm.exp2.f32(float)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0, !3}

!0 = distinct !{ptr @fmul6, !1}
!1 = distinct !{!2}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{ptr @fadd6, !1}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4, !11}
!6 = !{!"FuncMDMap[0]", ptr @fmul6}
!7 = !{!"FuncMDValue[1]", !4, !11}
!8 = !{!"FuncMDMap[1]", ptr @fadd6}
!9 = !{!"FuncMD", !6, !5, !8, !7}
!10 = !{!"ModuleMD", !9}
!IGCMetadata = !{!10}
!11 = !{!"resAllocMD", !12}
!12 = !{!"argAllocMDList", !13}
!13 = !{!"argAllocMDListVec[0]", !14, !15, !16}
!14 = !{!"type", i32 0}
!15 = !{!"extensionType", i32 -1}
!16 = !{!"indexType", i32 -1}
