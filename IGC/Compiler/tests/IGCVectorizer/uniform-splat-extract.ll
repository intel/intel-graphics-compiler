;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt -S --opaque-pointers -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 %s -o /dev/null | FileCheck %s

; CHECK: .decl s v_type=G type=f num_elts=1
; CHECK-NOT: s(0,{{[1-7]}})
; CHECK: lsc_store.ugm

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo(<8 x float> %v, float %s, ptr addrspace(1) %out) {
entry:
  %splat = call <8 x float> @llvm.genx.GenISA.Splat.v8f32.f32(float %s)
  %m = fmul <8 x float> %v, %splat
  %e = extractelement <8 x float> %splat, i32 5
  store float %e, ptr addrspace(1) %out, align 4
  %b = bitcast <8 x float> %m to <8 x i32>
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
!13 = !{!"FuncMDValue[0]", !16, !17}
!14 = !{!"FuncMD", !12, !13}
!15 = !{!"ModuleMD", !14}
!IGCMetadata = !{!15}
!16 = !{!"requiredSubGroupSize", i32 16}
!17 = !{!"resAllocMD", !18}
!18 = !{!"argAllocMDList", !22, !23, !24}
!19 = !{!"type", i32 0}
!20 = !{!"extensionType", i32 -1}
!21 = !{!"indexType", i32 -1}
!22 = !{!"argAllocMDListVec[0]", !19, !20, !21}
!23 = !{!"argAllocMDListVec[1]", !19, !20, !21}
!24 = !{!"argAllocMDListVec[2]", !19, !20, !21}
