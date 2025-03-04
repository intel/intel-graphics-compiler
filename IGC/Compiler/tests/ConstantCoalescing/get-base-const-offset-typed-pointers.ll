;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers --igc-constant-coalescing -dce -S < %s | FileCheck %s
; ------------------------------------------------
; ConstantCoalescing
; ------------------------------------------------

; Test checks const addr space loads merging:
; This test is for covering getPointerBaseWithConstantOffset function
; which is part of DecomposePtrExp, used to calculate base from bitcast
; or a getelementptr instruction.

; Merge
define void @test_merge(i32 addrspace(2)* %src) {
; CHECK-LABEL: define void @test_merge(
; CHECK-SAME: i32 addrspace(2)* [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = ptrtoint i32 addrspace(2)* [[SRC]] to i64
; CHECK-NEXT:    [[CHUNKPTR:%.*]] = inttoptr i64 [[TMP1]] to <2 x i32> addrspace(2)*
; CHECK-NEXT:    [[TMP2:%.*]] = load <2 x i32>, <2 x i32> addrspace(2)* [[CHUNKPTR]], align 4
; CHECK-NEXT:    [[TMP3:%.*]] = extractelement <2 x i32> [[TMP2]], i32 0
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <2 x i32> [[TMP2]], i32 1
; CHECK-NEXT:    call void @use.i32(i32 [[TMP3]])
; CHECK-NEXT:    call void @use.i32(i32 [[TMP4]])
; CHECK-NEXT:    ret void
;
  %1 = getelementptr i32, i32 addrspace(2)* %src, i32 1
  %2 = load i32, i32 addrspace(2)* %src
  %3 = load i32, i32 addrspace(2)* %1
  call void @use.i32(i32 %2)
  call void @use.i32(i32 %3)
  ret void
}

; TODO: check, these are potentially worse
define void @test_vectorize(float addrspace(2)* %src) {
; CHECK-LABEL: define void @test_vectorize(
; CHECK-SAME: float addrspace(2)* [[SRC:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast float addrspace(2)* [[SRC]] to i32 addrspace(2)*
; CHECK-NEXT:    [[TMP2:%.*]] = getelementptr i32, i32 addrspace(2)* [[TMP1]], i32 2
; CHECK-NEXT:    [[TMP3:%.*]] = ptrtoint float addrspace(2)* [[SRC]] to i64
; CHECK-NEXT:    [[CHUNKPTR:%.*]] = inttoptr i64 [[TMP3]] to <1 x i32> addrspace(2)*
; CHECK-NEXT:    [[TMP4:%.*]] = load <1 x i32>, <1 x i32> addrspace(2)* [[CHUNKPTR]], align 4
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <1 x i32> [[TMP4]], i32 0
; CHECK-NEXT:    [[TMP6:%.*]] = load i32, i32 addrspace(2)* [[TMP2]], align 4
; CHECK-NEXT:    call void @use.i32(i32 [[TMP5]])
; CHECK-NEXT:    call void @use.i32(i32 [[TMP6]])
; CHECK-NEXT:    ret void
;
  %1 = bitcast float addrspace(2)* %src to i32 addrspace(2)*
  %2 = getelementptr i32, i32 addrspace(2)* %1, i32 2
  %3 = load i32, i32 addrspace(2)* %1
  %4 = load i32, i32 addrspace(2)* %2
  call void @use.i32(i32 %3)
  call void @use.i32(i32 %4)
  ret void
}

define void @test_nonconst_gep(i32 addrspace(2)* %src, i32 %off) {
; CHECK-LABEL: define void @test_nonconst_gep(
; CHECK-SAME: i32 addrspace(2)* [[SRC:%.*]], i32 [[OFF:%.*]]) {
; CHECK-NEXT:    [[TMP1:%.*]] = getelementptr i32, i32 addrspace(2)* [[SRC]], i32 [[OFF]]
; CHECK-NEXT:    [[TMP2:%.*]] = ptrtoint i32 addrspace(2)* [[TMP1]] to i64
; CHECK-NEXT:    [[CHUNKPTR:%.*]] = inttoptr i64 [[TMP2]] to <2 x i32> addrspace(2)*
; CHECK-NEXT:    [[TMP3:%.*]] = load <2 x i32>, <2 x i32> addrspace(2)* [[CHUNKPTR]], align 4
; CHECK-NEXT:    [[TMP4:%.*]] = extractelement <2 x i32> [[TMP3]], i32 0
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP3]], i32 1
; CHECK-NEXT:    call void @use.i32(i32 [[TMP4]])
; CHECK-NEXT:    call void @use.i32(i32 [[TMP5]])
; CHECK-NEXT:    ret void
;
  %1 = getelementptr i32, i32 addrspace(2)* %src, i32 %off
  %2 = getelementptr i32, i32 addrspace(2)* %1, i32 1
  %3 = load i32, i32 addrspace(2)* %1
  %4 = load i32, i32 addrspace(2)* %2
  call void @use.i32(i32 %3)
  call void @use.i32(i32 %4)
  ret void
}

declare void @use.i32(i32)

!igc.functions = !{!0, !4, !5}

!0 = !{void (i32 addrspace(2)*)* @test_merge, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{void (float addrspace(2)*)* @test_vectorize, !1}
!5 = !{void (i32 addrspace(2)*, i32)* @test_nonconst_gep, !1}
