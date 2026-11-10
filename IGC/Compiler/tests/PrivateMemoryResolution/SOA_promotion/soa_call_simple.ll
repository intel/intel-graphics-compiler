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

; Check that SoA promotion is correctly applied across calls.

; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK-NOT: = alloca
; CHECK: [[SIMD:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK: [[BASE:%.*]] = ptrtoint ptr %privateBase to i64
; CHECK: [[LANEPTR:%.*]] = inttoptr i64 {{%.*}} to ptr
; CHECK: call void @callee(ptr [[LANEPTR]], i32 %i)

; CHECK-LABEL: define internal void @callee(
; CHECK: [[CSIMD:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK: [[CBASE:%.*]] = ptrtoint ptr %p to i64
; CHECK: lshr i32 {{%.*}}, 2
; CHECK: mul i32 [[CSIMD]],
; CHECK: add {{.*}} i64 [[CBASE]],
; CHECK: inttoptr
; CHECK: store float
define spir_kernel void @kernel(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
  %a = alloca [64 x float], align 4
  %i = extractelement <8 x i32> %payloadHeader, i64 0
  call void @callee(ptr %a, i32 %i)
  ret void
}

define internal void @callee(ptr %p, i32 %i) {
  %g = getelementptr inbounds float, ptr %p, i32 %i
  store float 1.0, ptr %g, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 false}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @kernel}
!5 = !{!"FuncMDValue[0]", !20}
!6 = !{ptr @kernel, !408}
!7 = !{ptr @callee, !450}
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
