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
; RUN: igc_opt --opaque-pointers --ocl --igc-private-mem-resolution --platformpvc \
; RUN:   --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableAggressiveSOAPromotion=0 -S %s \
; RUN:   | FileCheck %s --check-prefix=NOAGGR

; Verify that pointer comparisons on a SoA-promoted alloca are correctly rewritten to comparisons of logical byte offsets.

; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK-NOT: = alloca
; CHECK-NOT: icmp {{.*}} ptr
; CHECK: [[OFFI:%.*]] = mul nsw i32 %i, 4
; CHECK: [[OFFJ:%.*]] = mul nsw i32 %j, 4
; CHECK: [[LT:%.*]] = icmp ult i32 [[OFFI]], [[OFFJ]]
; CHECK: [[SAME:%.*]] = icmp eq i32 [[OFFI]], [[OFFI]]
; CHECK: [[S0:%.*]] = select i1 [[LT]], float {{%.*}}, float 0.000000e+00
; CHECK: [[S1:%.*]] = select i1 false, float 2.000000e+00, float [[S0]]
; CHECK: select i1 [[SAME]], float [[S1]], float 3.000000e+00
; CHECK-NOT: icmp {{.*}} ptr

; Without EnableAggressiveSOAPromotion pointer comparisons keep the alloca out of SoA.
; NOAGGR-LABEL: define spir_kernel void @kernel(
; NOAGGR: icmp ult ptr
; NOAGGR: icmp eq ptr

define spir_kernel void @kernel(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
  %a = alloca [64 x float], align 4
  %i = extractelement <8 x i32> %payloadHeader, i64 0
  %j = extractelement <8 x i32> %payloadHeader, i64 1
  %pi = getelementptr inbounds [64 x float], ptr %a, i32 0, i32 %i
  %pj = getelementptr inbounds [64 x float], ptr %a, i32 0, i32 %j
  store float 1.0, ptr %pi, align 4
  %lt = icmp ult ptr %pi, %pj
  %isnull = icmp eq ptr %pj, null
  %same = icmp eq ptr %pi, %pi
  %v = load float, ptr %pj, align 4
  %s0 = select i1 %lt, float %v, float 0.0
  %s1 = select i1 %isnull, float 2.0, float %s0
  %s2 = select i1 %same, float %s1, float 3.0
  store float %s2, ptr addrspace(1) %d, align 4
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
