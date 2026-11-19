;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 %s -o /dev/null | FileCheck %s

; CHECK: .decl sx v_type=G type=f num_elts=1
; CHECK-NOT: sx(0,{{[1-7]}})
; CHECK: ret

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo(float %s) {
entry:
  %sx = fmul float %s, 2.0
  %t0 = fptrunc float %sx to half
  %t1 = fptrunc float %sx to half
  %t2 = fptrunc float %sx to half
  %t3 = fptrunc float %sx to half
  %t4 = fptrunc float %sx to half
  %t5 = fptrunc float %sx to half
  %t6 = fptrunc float %sx to half
  %t7 = fptrunc float %sx to half
  %i0 = insertelement <8 x half> undef, half %t0, i32 0
  %i1 = insertelement <8 x half> %i0, half %t1, i32 1
  %i2 = insertelement <8 x half> %i1, half %t2, i32 2
  %i3 = insertelement <8 x half> %i2, half %t3, i32 3
  %i4 = insertelement <8 x half> %i3, half %t4, i32 4
  %i5 = insertelement <8 x half> %i4, half %t5, i32 5
  %i6 = insertelement <8 x half> %i5, half %t6, i32 6
  %i7 = insertelement <8 x half> %i6, half %t7, i32 7
  %b = bitcast <8 x half> %i7 to <8 x i16>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 16, i32 8, i32 1, i32 1, i1 false, i1 false, i32 0, <8 x i16> %b)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i16>)
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
