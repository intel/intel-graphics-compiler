;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --igc-memopt --regkey=EnableSubDWordMergeAlignmentCheck=0 -S %s | FileCheck %s --check-prefixes=CHECK,DISABLED
; RUN: igc_opt --opaque-pointers --igc-memopt --regkey=EnableSubDWordMergeAlignmentCheck=1 -S %s | FileCheck %s --check-prefixes=CHECK,ENABLED

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i16 @align2(
; DISABLED-NOT: call i16 @llvm.genx.GenISA.PredicatedLoad
; DISABLED: call <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p1.v2i16(ptr addrspace(1) {{.*}}, i64 2, i1 %pred, <2 x i16> zeroinitializer)
; ENABLED-NOT: call <2 x i16>
; ENABLED: %a = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %src, i64 2, i1 %pred, i16 0)
; ENABLED: %b = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %next, i64 2, i1 %pred, i16 0)
; CHECK-NOT: PredicatedLoad
; CHECK: ret i16
define i16 @align2(ptr addrspace(1) %base, i1 %pred) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %lane to i64
  %offset = shl i64 %index, 1
  %src = getelementptr i16, ptr addrspace(1) %base, i64 %offset
  %next = getelementptr i16, ptr addrspace(1) %src, i64 1
  %a = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %src, i64 2, i1 %pred, i16 0)
  %b = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %next, i64 2, i1 %pred, i16 0)
  %sum = add i16 %a, %b
  ret i16 %sum
}

; The leading pointer is DWORD aligned; the adjacent i16 is only 2-byte aligned.
; CHECK-LABEL: define i16 @align4(
; CHECK-NOT: call i16 @llvm.genx.GenISA.PredicatedLoad
; CHECK: call <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p1.v2i16(ptr addrspace(1) {{.*}}, i64 4, i1 %pred, <2 x i16> zeroinitializer)
; CHECK-NOT: PredicatedLoad
; CHECK: ret i16
define i16 @align4(ptr addrspace(1) %base, i1 %pred) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %lane to i64
  %offset = shl i64 %index, 1
  %src = getelementptr i16, ptr addrspace(1) %base, i64 %offset
  %next = getelementptr i16, ptr addrspace(1) %src, i64 1
  %a = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %src, i64 4, i1 %pred, i16 0)
  %b = call i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1) %next, i64 2, i1 %pred, i16 0)
  %sum = add i16 %a, %b
  ret i16 %sum
}

declare i16 @llvm.genx.GenISA.simdLaneId()

declare i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1), i64, i1, i16) #0
attributes #0 = { nounwind readonly }

!igc.functions = !{!0, !1}
!0 = !{ptr @align2, !2}
!1 = !{ptr @align4, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
