;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -regkey EnableAtomicBranch=1 -igc-InsertBranchOpt -S < %s | FileCheck %s
; ------------------------------------------------
; InsertBranchOpt: Atomic Split
; ------------------------------------------------

define void @test_atomic_split(ptr %src1) {
; CHECK-LABEL: @test_atomic_split(
; CHECK:    [[TMP1:%.*]] = load i32, ptr [[SRC1:%.*]], align 4
; CHECK:    [[TMP2:%.*]] = icmp ne i32 [[TMP1]], 0
; CHECK:    br i1 [[TMP2]], label %[[ATOMIC_IF_TRUE:.*]], label %[[ATOMIC_IF_FALSE:.*]]
; CHECK:  [[ATOMIC_IF_TRUE]]:
; CHECK:    [[TMP3:%.*]] = call i32 @llvm.genx.GenISA.intatomictyped.i32.p0(ptr [[SRC1]], i32 1, i32 2, i32 3, i32 [[TMP1]], i32 1)
; CHECK:    br label %[[ATOMIC_IF_END:.*]]
; CHECK:  [[ATOMIC_IF_FALSE]]:
; CHECK:    [[TMP4:%.*]] = call <4 x float> @llvm.genx.GenISA.typedread.p0(ptr [[SRC1]], i32 1, i32 2, i32 3, i32 0)
; CHECK:    [[TMP5:%.*]] = extractelement <4 x float> [[TMP4]], i64 0
; CHECK:    [[TMP6:%.*]] = bitcast float [[TMP5]] to i32
; CHECK:    br label %[[ATOMIC_IF_END]]
; CHECK:  [[ATOMIC_IF_END]]:
; CHECK:    [[TMP7:%.*]] = phi i32 [ [[TMP3]], %[[ATOMIC_IF_TRUE]] ], [ [[TMP6]], %[[ATOMIC_IF_FALSE]] ]
; CHECK:    call void @use.i32(i32 [[TMP7]])
; CHECK:    ret void
;
  %1 = load i32, ptr %src1, align 4
  %2 = call i32 @llvm.genx.GenISA.intatomictyped.i32.p0(ptr %src1, i32 1, i32 2, i32 3, i32 %1, i32 1)
  call void @use.i32(i32 %2)
  ret void
}

declare i32 @llvm.genx.GenISA.intatomictyped.i32.p0(ptr, i32, i32, i32, i32, i32)
declare void @use.i32(i32)
