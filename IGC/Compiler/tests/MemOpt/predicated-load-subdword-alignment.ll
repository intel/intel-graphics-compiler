;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -platformbmg --igc-memopt --regkey=EnableSubDWordMergeAlignmentCheck=0 -S %s | FileCheck %s --check-prefixes=CHECK,DISABLED
; RUN: igc_opt --opaque-pointers -platformbmg --igc-memopt --regkey=EnableSubDWordMergeAlignmentCheck=1 -S %s | FileCheck %s --check-prefixes=CHECK,ENABLED

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

; Don't block if the merged access is below 4 bytes.
; CHECK-LABEL: define i8 @bytes2(
; CHECK: load <2 x i8>, ptr addrspace(1) %src, align 1
; CHECK-NOT: load i8
; CHECK: ret i8
define i8 @bytes2(ptr addrspace(1) %base) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %lane to i64
  %offset = shl i64 %index, 1
  %src = getelementptr i8, ptr addrspace(1) %base, i64 %offset
  %next = getelementptr i8, ptr addrspace(1) %src, i64 1
  %a = load i8, ptr addrspace(1) %src, align 1
  %b = load i8, ptr addrspace(1) %next, align 1
  %sum = add i8 %a, %b
  ret i8 %sum
}

; Don't block SLM.
; CHECK-LABEL: define i16 @slm4(
; CHECK: load <4 x i8>, ptr addrspace(3) %src, align 1
; CHECK-NOT: load i8
; CHECK: ret i16
define i16 @slm4(ptr addrspace(3) %base) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %lane to i32
  %offset = shl i32 %index, 2
  %src = getelementptr i8, ptr addrspace(3) %base, i32 %offset
  %p1 = getelementptr i8, ptr addrspace(3) %src, i32 1
  %p2 = getelementptr i8, ptr addrspace(3) %src, i32 2
  %p3 = getelementptr i8, ptr addrspace(3) %src, i32 3
  %a = load i8, ptr addrspace(3) %src, align 1
  %b = load i8, ptr addrspace(3) %p1, align 1
  %c = load i8, ptr addrspace(3) %p2, align 1
  %d = load i8, ptr addrspace(3) %p3, align 1
  %ab = add i8 %a, %b
  %cd = add i8 %c, %d
  %s = add i8 %ab, %cd
  %r = zext i8 %s to i16
  ret i16 %r
}

; 4x reduction in messages is profitable, don't block.
; CHECK-LABEL: define i16 @bytes8(
; CHECK: load <8 x i8>, ptr addrspace(1) %src, align 1
; CHECK-NOT: load i8
; CHECK: ret i16
define i16 @bytes8(ptr addrspace(1) %base) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %lane to i64
  %offset = shl i64 %index, 3
  %src = getelementptr i8, ptr addrspace(1) %base, i64 %offset
  %p1 = getelementptr i8, ptr addrspace(1) %src, i64 1
  %p2 = getelementptr i8, ptr addrspace(1) %src, i64 2
  %p3 = getelementptr i8, ptr addrspace(1) %src, i64 3
  %p4 = getelementptr i8, ptr addrspace(1) %src, i64 4
  %p5 = getelementptr i8, ptr addrspace(1) %src, i64 5
  %p6 = getelementptr i8, ptr addrspace(1) %src, i64 6
  %p7 = getelementptr i8, ptr addrspace(1) %src, i64 7
  %a = load i8, ptr addrspace(1) %src, align 1
  %b = load i8, ptr addrspace(1) %p1, align 1
  %c = load i8, ptr addrspace(1) %p2, align 1
  %d = load i8, ptr addrspace(1) %p3, align 1
  %e = load i8, ptr addrspace(1) %p4, align 1
  %f = load i8, ptr addrspace(1) %p5, align 1
  %g = load i8, ptr addrspace(1) %p6, align 1
  %h = load i8, ptr addrspace(1) %p7, align 1
  %s0 = add i8 %a, %b
  %s1 = add i8 %s0, %c
  %s2 = add i8 %s1, %d
  %s3 = add i8 %s2, %e
  %s4 = add i8 %s3, %f
  %s5 = add i8 %s4, %g
  %s6 = add i8 %s5, %h
  %r = zext i8 %s6 to i16
  ret i16 %r
}

; 2x reduction in messages is not profitable, block.
; CHECK-LABEL: define i16 @words4(
; DISABLED: load <4 x i16>, ptr addrspace(1) %src, align 2
; ENABLED-NOT: load <4 x i16>
; CHECK: ret i16
define i16 @words4(ptr addrspace(1) %base) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %lane to i64
  %offset = shl i64 %index, 2
  %src = getelementptr i16, ptr addrspace(1) %base, i64 %offset
  %p1 = getelementptr i16, ptr addrspace(1) %src, i64 1
  %p2 = getelementptr i16, ptr addrspace(1) %src, i64 2
  %p3 = getelementptr i16, ptr addrspace(1) %src, i64 3
  %a = load i16, ptr addrspace(1) %src, align 2
  %b = load i16, ptr addrspace(1) %p1, align 2
  %c = load i16, ptr addrspace(1) %p2, align 2
  %d = load i16, ptr addrspace(1) %p3, align 2
  %ab = add i16 %a, %b
  %cd = add i16 %c, %d
  %s = add i16 %ab, %cd
  ret i16 %s
}

declare i16 @llvm.genx.GenISA.simdLaneId()

declare i16 @llvm.genx.GenISA.PredicatedLoad.i16.p1.i16(ptr addrspace(1), i64, i1, i16) #0
attributes #0 = { nounwind readonly }

!igc.functions = !{!0, !1, !4, !5, !6, !7}
!0 = !{ptr @align2, !2}
!1 = !{ptr @align4, !2}
!4 = !{ptr @bytes2, !2}
!5 = !{ptr @slm4, !2}
!6 = !{ptr @bytes8, !2}
!7 = !{ptr @words4, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
