;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; ------------------------------------------------
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-gep-lowering -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GEPLowering - verify that address arithmetic based on first GEP index is not
;               applied, when GEP index can be negative and zext if used on
;               negative index may turn it into large positive value
; ------------------------------------------------

define spir_func void @test_gep_overflow(ptr addrspace(1) %src, i32 %conv13) {
; CHECK-LABEL: @test_gep_overflow(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[SUB:%.*]] = add nsw i32 [[CONV13:%.*]], -2
; CHECK-NEXT:    [[IDX_EXT19:%.*]] = zext i32 [[SUB]] to i64
; CHECK-NEXT:    [[TMP0:%.*]] = ptrtoint ptr addrspace(1) [[SRC:%.*]] to i64
; CHECK-NEXT:    [[TMP1:%.*]] = shl i64 [[IDX_EXT19]], 2
; CHECK-NEXT:    [[TMP2:%.*]] = add i64 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = inttoptr i64 [[TMP2]] to ptr addrspace(1)
; CHECK-NEXT:    [[SUB22:%.*]] = add nsw i32 [[CONV13]], -1
; CHECK-NEXT:    [[IDX_EXT35:%.*]] = zext i32 [[SUB22]] to i64
; CHECK-NEXT:    [[TMP4:%.*]] = ptrtoint ptr addrspace(1) [[SRC]] to i64
; CHECK-NEXT:    [[TMP5:%.*]] = shl i64 [[IDX_EXT35]], 2
; CHECK-NEXT:    [[TMP6:%.*]] = add i64 [[TMP4]], [[TMP5]]
; CHECK-NEXT:    [[TMP7:%.*]] = inttoptr i64 [[TMP6]] to ptr addrspace(1)
; CHECK-NEXT:    ret void
;
entry:
  %sub = add nsw i32 %conv13, -2
  %idx.ext19 = zext i32 %sub to i64
  %add.ptr20 = getelementptr inbounds float, ptr addrspace(1) %src, i64 %idx.ext19
  %sub22 = add nsw i32 %conv13, -1
  %idx.ext35 = zext i32 %sub22 to i64
  %add.ptr36 = getelementptr inbounds float, ptr addrspace(1) %src, i64 %idx.ext35
  ret void
}

define spir_func void @test_gep_no_overflow(ptr addrspace(1) %src, i32 %conv13) {
; CHECK-LABEL: @test_gep_no_overflow(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[SUB:%.*]] = add nuw nsw i32 [[CONV13:%.*]], -2
; CHECK-NEXT:    [[IDX_EXT19:%.*]] = zext i32 [[SUB]] to i64
; CHECK-NEXT:    [[TMP0:%.*]] = ptrtoint ptr addrspace(1) [[SRC:%.*]] to i64
; CHECK-NEXT:    [[TMP1:%.*]] = shl i64 [[IDX_EXT19]], 2
; CHECK-NEXT:    [[TMP2:%.*]] = add i64 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = inttoptr i64 [[TMP2]] to ptr addrspace(1)
; CHECK-NEXT:    [[TMP4:%.*]] = add i64 [[TMP2]], 4
; CHECK-NEXT:    [[TMP5:%.*]] = inttoptr i64 [[TMP4]] to ptr addrspace(1)
; CHECK-NEXT:    ret void
;
entry:
  %sub = add nsw nuw i32 %conv13, -2
  %idx.ext19 = zext i32 %sub to i64
  %add.ptr20 = getelementptr inbounds float, ptr addrspace(1) %src, i64 %idx.ext19
  %sub22 = add nsw nuw i32 %conv13, -1
  %idx.ext35 = zext i32 %sub22 to i64
  %add.ptr36 = getelementptr inbounds float, ptr addrspace(1) %src, i64 %idx.ext35
  ret void
}

!igc.functions = !{!0, !4}

!0 = !{ptr @test_gep_overflow, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{ptr @test_gep_no_overflow, !1}
