;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -platformbmg -GenXCodeGenModule \
; RUN:   --regkey DisableCodeScheduling=0 --regkey CodeSchedulingAutoGRFEager=1 \
; RUN:   --regkey CodeSchedulingForceRPOnly=1 \
; RUN:   --regkey AllowSIMD16DropForXE2Plus=0 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 | FileCheck %s
;
; The callee has no required SIMD width of its own. Original-order analysis,
; candidate construction, and fit checks must all use the group head's SIMD16.
;
; CHECK-LABEL: Function callee
; CHECK: Auto GRF original: block=entry, initialRP=[[INITIAL:[0-9]+]], maxRP={{[0-9]+}}, admissionGRFTarget=128
; CHECK: Building scheduling dependency graph for entry
; CHECK: SIMD: 16
; CHECK: Initial CurrentPressure in registers: [[INITIAL]]
; CHECK: Auto GRF candidate: block=entry, grfTarget=128, fitsWithNoSpills=1

define spir_kernel void @kernel(ptr addrspace(1) %dst, <8 x i32> %x) {
entry:
  call spir_func void @callee(ptr addrspace(1) %dst, <8 x i32> %x)
  ret void
}

define internal spir_func void @callee(ptr addrspace(1) %dst, <8 x i32> %x) #0 {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> %x,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 4
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
    <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

attributes #0 = { "num-thread-per-eu"="0" }

!igc.functions = !{!0, !1}
!0 = !{ptr @kernel, !2}
!1 = !{ptr @callee, !3}
!2 = !{!{!"function_type", i32 0}}
!3 = !{!{!"function_type", i32 2}}

!IGCMetadata = !{!4}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", ptr @kernel}
!7 = !{!"FuncMDValue[0]", !8}
!8 = !{!"requiredSubGroupSize", i32 16}
