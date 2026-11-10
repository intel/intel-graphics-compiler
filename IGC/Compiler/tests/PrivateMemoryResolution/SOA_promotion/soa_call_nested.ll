;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers --ocl --igc-private-mem-resolution --platformpvc \
; RUN:   --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s

; Check that SoA promotion is correctly applied across nested calls.

; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK-NOT: = alloca
; CHECK: call void @mid(ptr {{%.*}}, i32 %i)

; CHECK-LABEL: define internal void @mid(
; CHECK-NOT: inttoptr
; CHECK: call void @leaf(ptr %p, i32 %j)

; CHECK-LABEL: define internal void @leaf(
; CHECK: [[SIMD:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK: [[BASE:%.*]] = ptrtoint ptr %q to i64
; CHECK: mul i32 [[SIMD]],
; CHECK: add {{.*}} i64 [[BASE]],
; CHECK: load float
define spir_kernel void @kernel(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
  %a = alloca [64 x float], align 4
  %i = extractelement <8 x i32> %payloadHeader, i64 0
  call void @mid(ptr %a, i32 %i)
  ret void
}

define internal void @mid(ptr %p, i32 %j) {
  call void @leaf(ptr %p, i32 %j)
  ret void
}

define internal void @leaf(ptr %q, i32 %k) {
  %g = getelementptr inbounds float, ptr %q, i32 %k
  %v = load float, ptr %g, align 4
  %s = fadd float %v, 1.0
  store float %s, ptr %g, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!500, !520, !521}
!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 false}
!3 = !{!"FuncMD", !10, !11}
!10 = !{!"FuncMDMap[0]", ptr @kernel}
!11 = !{!"FuncMDValue[0]", !20}
!20 = !{!"implicitArgInfoList", !21, !22, !23, !24, !25, !26, !27}
!21 = !{!"implicitArgInfoListVec[0]", !30}
!22 = !{!"implicitArgInfoListVec[1]", !31}
!23 = !{!"implicitArgInfoListVec[2]", !32}
!24 = !{!"implicitArgInfoListVec[3]", !33}
!25 = !{!"implicitArgInfoListVec[4]", !34}
!26 = !{!"implicitArgInfoListVec[5]", !35}
!27 = !{!"implicitArgInfoListVec[6]", !36}
!30 = !{!"argId", i32 0}
!31 = !{!"argId", i32 1}
!32 = !{!"argId", i32 7}
!33 = !{!"argId", i32 8}
!34 = !{!"argId", i32 9}
!35 = !{!"argId", i32 10}
!36 = !{!"argId", i32 13}
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
!450 = !{!451}
!451 = !{!"function_type", i32 1}
!500 = !{ptr @kernel, !408}
!520 = !{ptr @mid, !450}
!521 = !{ptr @leaf, !450}
