;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys,llvm-16-plus
;
; RUN: igc_opt --ocl --igc-private-mem-resolution --regkey EnablePrivMemNewSOATranspose=1 --regkey EnableOpaquePointersBackend=1 -S %s | FileCheck %s
;
; Regression test: the SOA layout checker's same-size reinterpret relaxation
; must NOT apply under UseScratchSpacePrivateMemoryOrUseStatelessStrategy.
; The legacy TransposeHelperPrivateMem asserts that scalar lane size == element
; size, so promoting [3 x i32] accessed as <2 x half> would trigger that assert.
; The alloca must remain un-transposed (non-SOA) under the scratch space path.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_no_reinterpret(ptr nocapture writeonly %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr nocapture readnone %privateBase) {
entry:
; The alloca must NOT be SOA-transposed: the load type (<2 x half>) has a
; different scalar size than the alloca element type (i32), even though the
; total store size matches (32 bits).  Under the scratch space strategy the
; legacy transpose helper cannot handle this.
;
; CHECK-LABEL: @test_no_reinterpret(
; CHECK: load <2 x half>, ptr %{{.*}}
; CHECK-NOT: insertelement <3 x i32>
  %arr = alloca [3 x i32], align 4
  %gep = getelementptr inbounds [3 x i32], ptr %arr, i64 0, i64 1
  %val = load <2 x half>, ptr %gep, align 4
  store <2 x half> %val, ptr %out, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[1]", ptr @test_no_reinterpret}
!5 = !{!"FuncMDValue[1]", !2}
!6 = !{ptr @test_no_reinterpret, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !413, !414, !415, !416, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!413 = !{i32 7}
!414 = !{i32 8}
!415 = !{i32 9}
!416 = !{i32 10}
!417 = !{i32 13}
