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
; RUN:   --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableAggressiveSOAPromotion=1 \
; RUN:   --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s

; Verify that SoA promotion is done and that comparisons between pointers derived from the same
; promoted argument are correctly handled.

; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK: call void @callee(ptr {{%.*}}, i32 %i, i32 %j, ptr addrspace(1) %d)
; CHECK: call void @callee_two(ptr {{%.*}}, ptr {{%.*}}, ptr addrspace(1) %d)

; CHECK-LABEL: define internal void @callee(
; CHECK: ptrtoint ptr %p to i64
; CHECK-NOT: icmp {{.*}} ptr
; CHECK: [[OFFI:%.*]] = mul nsw i32 %i, 4
; CHECK: [[OFFJ:%.*]] = mul nsw i32 %j, 4
; CHECK: icmp ule i32 [[OFFI]], [[OFFJ]]
; CHECK: select i1 false,
; CHECK-NOT: icmp {{.*}} ptr

; CHECK-LABEL: define internal void @callee_two(
; CHECK-NOT: ptrtoint
; CHECK: icmp eq ptr %p, %q
define spir_kernel void @kernel(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
  %a = alloca [64 x float], align 4
  %b = alloca [64 x float], align 4
  %c = alloca [64 x float], align 4
  %i = extractelement <8 x i32> %payloadHeader, i64 0
  %j = extractelement <8 x i32> %payloadHeader, i64 1
  call void @callee(ptr %a, i32 %i, i32 %j, ptr addrspace(1) %d)
  call void @callee_two(ptr %b, ptr %c, ptr addrspace(1) %d)
  ret void
}

define internal void @callee(ptr %p, i32 %i, i32 %j, ptr addrspace(1) %d) {
  %pi = getelementptr inbounds float, ptr %p, i32 %i
  %pj = getelementptr inbounds float, ptr %p, i32 %j
  store float 1.0, ptr %pi, align 4
  %le = icmp ule ptr %pi, %pj
  %isnull = icmp eq ptr %p, null
  %v = load float, ptr %pj, align 4
  %s0 = select i1 %le, float %v, float 0.0
  %s1 = select i1 %isnull, float 2.0, float %s0
  store float %s1, ptr addrspace(1) %d, align 4
  ret void
}

define internal void @callee_two(ptr %p, ptr %q, ptr addrspace(1) %d) {
  store float 1.0, ptr %p, align 4
  store float 2.0, ptr %q, align 4
  %same = icmp eq ptr %p, %q
  %v = select i1 %same, float 1.0, float 0.0
  store float %v, ptr addrspace(1) %d, align 4
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
!520 = !{ptr @callee, !450}
!521 = !{ptr @callee_two, !450}
