;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s --opaque-pointers -S -o - -types-legalization-pass -dce | FileCheck %s
; REQUIRES: llvm-14-plus


; CHECK-LABEL: define spir_kernel void @test_ResolveStoreArgStruct(%nested.struct.type %in, ptr %out_ptr) {
; CHECK:         [[T1:%.*]] = extractvalue %nested.struct.type %in, 0, 0
; CHECK-NEXT:    [[T2:%.*]] = getelementptr %nested.struct.type, ptr %out_ptr, i32 0, i32 0, i32 0
; CHECK-NEXT:    store i32 [[T1]], ptr [[T2]], align 4
; CHECK-NEXT:    [[T3:%.*]] = extractvalue %nested.struct.type %in, 0, 1
; CHECK-NEXT:    [[T4:%.*]] = getelementptr %nested.struct.type, ptr %out_ptr, i32 0, i32 0, i32 1
; CHECK-NEXT:    store double [[T3]], ptr [[T4]], align 4
; CHECK-NEXT:    [[T5:%.*]] = extractvalue %nested.struct.type %in, 1, 0
; CHECK-NEXT:    [[T6:%.*]] = getelementptr %nested.struct.type, ptr %out_ptr, i32 0, i32 1, i32 0
; CHECK-NEXT:    store i32 [[T5]], ptr [[T6]], align 4
; CHECK-NEXT:    [[T7:%.*]] = extractvalue %nested.struct.type %in, 1, 1
; CHECK-NEXT:    [[T8:%.*]] = getelementptr %nested.struct.type, ptr %out_ptr, i32 0, i32 1, i32 1
; CHECK-NEXT:    store i32 [[T7]], ptr [[T8]], align 4


%simple.struct.type = type { i32, double }
%nested.struct.type = type { %simple.struct.type, [2 x i32] }

define spir_kernel void @test_ResolveStoreArgStruct(%nested.struct.type %in, ptr %out_ptr)
{
  store %nested.struct.type %in, ptr %out_ptr, align 4
  ret void
}
