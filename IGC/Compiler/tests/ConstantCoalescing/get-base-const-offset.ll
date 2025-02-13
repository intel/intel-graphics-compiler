;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-constant-coalescing -dce -S < %s | FileCheck %s
; ------------------------------------------------
; ConstantCoalescing
; ------------------------------------------------

; Test checks const addr space loads merging:
; This test is for covering getPointerBaseWithConstantOffset function
; which is part of DecomposePtrExp, used to calculate base from bitcast
; or a getelementptr instruction.

define void @test_merge(ptr addrspace(2) %src) {
; CHECK-LABEL: define void @test_merge(
; CHECK-SAME: ptr addrspace(2) [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = ptrtoint ptr addrspace(2) [[SRC]] to i64
; CHECK-NEXT:    [[CHUNKPTR:%.*]] = inttoptr i64 [[TMP1]] to ptr addrspace(2)
; CHECK-NEXT:    [[TMP2:%.*]] = load <2 x i32>, ptr addrspace(2) [[CHUNKPTR]], align 4
; CHECK-NEXT:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP2]], i32 0
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <2 x i32> [[TMP2]], i32 1
; CHECK-NEXT:    call void @use.i32(i32 [[TMP3]])
; CHECK-NEXT:    call void @use.i32(i32 [[TMP4]])
; CHECK-NEXT:    ret void
;
  %1 = getelementptr i32, ptr addrspace(2) %src, i32 1
  %2 = load i32, ptr addrspace(2) %src, align 4
  %3 = load i32, ptr addrspace(2) %1, align 4
  call void @use.i32(i32 %2)
  call void @use.i32(i32 %3)
  ret void
}

define void @test_vectorize(ptr addrspace(2) %src) {
; CHECK-LABEL: define void @test_vectorize(
; CHECK-SAME: ptr addrspace(2) [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = getelementptr i32, ptr addrspace(2) [[SRC]], i32 2
; CHECK-NEXT:    [[TMP2:%.*]] = ptrtoint ptr addrspace(2) [[SRC]] to i64
; CHECK-NEXT:    [[CHUNKPTR:%.*]] = inttoptr i64 [[TMP2]] to ptr addrspace(2)
; CHECK-NEXT:    [[TMP3:%.*]] = load <1 x i32>, ptr addrspace(2) [[CHUNKPTR]], align 4
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <1 x i32> [[TMP3]], i32 0
; CHECK-NEXT:    [[TMP5:%.*]] = load i32, ptr addrspace(2) [[TMP1]], align 4
; CHECK-NEXT:    call void @use.i32(i32 [[TMP4]])
; CHECK-NEXT:    call void @use.i32(i32 [[TMP5]])
; CHECK-NEXT:    ret void
;
  %1 = getelementptr i32, ptr addrspace(2) %src, i32 2
  %2 = load i32, ptr addrspace(2) %src, align 4
  %3 = load i32, ptr addrspace(2) %1, align 4
  call void @use.i32(i32 %2)
  call void @use.i32(i32 %3)
  ret void
}

define void @test_nonconst_gep(ptr addrspace(2) %src, i32 %off) {
; CHECK-LABEL: define void @test_nonconst_gep(
; CHECK-SAME: ptr addrspace(2) [[SRC:%.*]], i32 [[OFF:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = getelementptr i32, ptr addrspace(2) [[SRC]], i32 [[OFF]]
; CHECK-NEXT:    [[TMP2:%.*]] = ptrtoint ptr addrspace(2) [[TMP1]] to i64
; CHECK-NEXT:    [[CHUNKPTR:%.*]] = inttoptr i64 [[TMP2]] to ptr addrspace(2)
; CHECK-NEXT:    [[TMP3:%.*]] = load <2 x i32>, ptr addrspace(2) [[CHUNKPTR]], align 4
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <2 x i32> [[TMP3]], i32 0
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP3]], i32 1
; CHECK-NEXT:    call void @use.i32(i32 [[TMP4]])
; CHECK-NEXT:    call void @use.i32(i32 [[TMP5]])
; CHECK-NEXT:    ret void
;
  %1 = getelementptr i32, ptr addrspace(2) %src, i32 %off
  %2 = getelementptr i32, ptr addrspace(2) %1, i32 1
  %3 = load i32, ptr addrspace(2) %1, align 4
  %4 = load i32, ptr addrspace(2) %2, align 4
  call void @use.i32(i32 %3)
  call void @use.i32(i32 %4)
  ret void
}

declare void @use.i32(i32)

!igc.functions = !{!0, !4, !5}

!0 = !{ptr @test_merge, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{ptr @test_vectorize, !1}
!5 = !{ptr @test_nonconst_gep, !1}
