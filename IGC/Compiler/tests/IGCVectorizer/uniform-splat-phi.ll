;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt -S --opaque-pointers -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 %s -o /dev/null | FileCheck %s

; CHECK: _main_0:
; CHECK: splat(0,0)<1>
; CHECK: _foo_001_loop:
; CHECK: add (M1_NM, 8) splat(0,0)<1> splat(0,0)<1;1,0> v(0,0)<1;1,0>

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo(<8 x float> %v, float %s, i32 %n) {
entry:
  %splat = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %s)
  br label %loop

loop:
  %acc = phi <8 x float> [ %splat, %entry ], [ %next, %loop ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %next = fadd <8 x float> %acc, %v
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, %n
  br i1 %cond, label %loop, label %exit

exit:
  %b = bitcast <8 x float> %next to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 32, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i32> %b)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

!igc.functions = !{!0}
!0 = !{ptr @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!12 = !{!"FuncMDMap[0]", ptr @foo}
!13 = !{!"FuncMDValue[0]", !16}
!14 = !{!"FuncMD", !12, !13}
!15 = !{!"ModuleMD", !14}
!IGCMetadata = !{!15}
!16 = !{!"requiredSubGroupSize", i32 16}
