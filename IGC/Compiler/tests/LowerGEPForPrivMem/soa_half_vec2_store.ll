;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that the alloca is promoted to registers using the old SOA algorithm in case new algorithm is not applicable.

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --ocl -igc-priv-mem-to-reg --regkey EnablePrivMemNewSOATranspose=1,EnablePrivMemNewSOAForScalarArrays=1,EnableSOAFallbackToOldAlgorithm=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%half_t = type { half }
%vec_base = type { [4 x %half_t] }

; CHECK-LABEL: define spir_kernel void @test
; CHECK:       alloca <4 x half>
; CHECK-NOT:   alloca %vec_base
; CHECK:       store <4 x half>
; CHECK:       ret void

define spir_kernel void @test(ptr addrspace(1) %out, i64 %i, <2 x half> %v, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca %vec_base, align 2
  %idx = shl i64 %i, 1
  %p = getelementptr inbounds %half_t, ptr %a, i64 %idx
  store <2 x half> %v, ptr %p, align 2
  %l0 = load i16, ptr %a, align 2
  store i16 %l0, ptr addrspace(1) %out, align 2
  %g1 = getelementptr inbounds %half_t, ptr %a, i64 1
  %l1 = load i16, ptr %g1, align 2
  %o1 = getelementptr inbounds i16, ptr addrspace(1) %out, i64 1
  store i16 %l1, ptr addrspace(1) %o1, align 2
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!13}
!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test}
!5 = !{!"FuncMDValue[0]", !6}
!6 = !{!"implicitArgInfoList", !7, !9, !11}
!7 = !{!"implicitArgInfoListVec[0]", !8}
!8 = !{!"argId", i32 0}
!9 = !{!"implicitArgInfoListVec[1]", !10}
!10 = !{!"argId", i32 1}
!11 = !{!"implicitArgInfoListVec[2]", !12}
!12 = !{!"argId", i32 13}
!13 = !{ptr @test, !14}
!14 = !{!15}
!15 = !{!"function_type", i32 0}
