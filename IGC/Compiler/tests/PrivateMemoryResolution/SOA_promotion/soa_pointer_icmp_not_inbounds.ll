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
; RUN:   --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

; A pointer comparison on an alloca is only rewritten to a comparison of logical byte offsets when both operands are
; reached through inbounds GEPs, which keeps the offsets within the alloca and non-negative. Here one operand is
; derived with a GEP that is not inbounds, so the comparison is kept and the alloca stays AoS: a per-lane stride of
; the whole 256-byte array.

; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK:       [[LANE:%.*]] = zext i16 {{%.*}} to i32
; CHECK:       mul i32 [[LANE]], 256
; CHECK:       icmp ult ptr
; CHECK:       ret void

define spir_kernel void @kernel(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
  %a = alloca [64 x float], align 4
  %i = extractelement <8 x i32> %payloadHeader, i64 0
  %j = extractelement <8 x i32> %payloadHeader, i64 1
  %pi = getelementptr inbounds [64 x float], ptr %a, i32 0, i32 %i
  %pj = getelementptr [64 x float], ptr %a, i32 0, i32 %j
  store float 1.0, ptr %pi, align 4
  %lt = icmp ult ptr %pi, %pj
  %v = load float, ptr %pj, align 4
  %s0 = select i1 %lt, float %v, float 0.0
  store float %s0, ptr addrspace(1) %d, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!500}
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
