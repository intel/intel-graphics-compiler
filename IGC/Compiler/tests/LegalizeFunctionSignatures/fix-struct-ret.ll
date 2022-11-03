;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-legalize-function-signatures -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,%LLVM_DEPENDENT_CHECK_PREFIX%
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------

; Test checks that struct ret value is legalized

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS
%str = type { i32, i64 }

define spir_kernel void @test_k(i32 %src) {
; CHECK-LABEL: @test_k(
; CHECK:    [[TMP1:%.*]] = alloca [[STR:%[A-z0-9]*]]
; CHECK-LLVM-14-PLUS: call void @foo(%str* noalias sret([[STR]]) [[TMP1]], i32 [[SRC:%.*]], i64 13)
; CHECK-PRE-LLVM-14:  call void @foo(%str* noalias sret [[TMP1]], i32 [[SRC:%.*]], i64 13)
; CHECK:    [[TMP2:%.*]] = load [[STR]], %str* [[TMP1]]
; CHECK:    [[TMP3:%.*]] = extractvalue [[STR]] [[TMP2]], 0
; CHECK:    [[TMP4:%.*]] = extractvalue [[STR]] [[TMP2]], 1
; CHECK:    call void @use.i32(i32 [[TMP3]])
; CHECK:    call void @use.i64(i64 [[TMP4]])
; CHECK:    ret void
;
  %1 = call %str @foo(i32 %src, i64 13)
  %2 = extractvalue %str %1, 0
  %3 = extractvalue %str %1, 1
  call void @use.i32(i32 %2)
  call void @use.i64(i64 %3)
  ret void
}

define spir_func %str @foo(i32 %src1, i64 %src2) #0 {
; CHECK-LLVM-14-PLUS: define spir_func void @foo(%str* noalias sret(%str) %0, i32 %src1, i64 %src2)
; CHECK-PRE-LLVM-14: define spir_func void @foo(%str* noalias sret, i32 %src1, i64 %src2)
; CHECK:    [[TMP2:%.*]] = insertvalue [[STR:%.*]] { i32 42, i64 0 }, i32 [[SRC1:%.*]], 0
; CHECK:    [[TMP3:%.*]] = insertvalue [[STR]] [[TMP2]], i64 [[SRC2:%.*]], 1
; CHECK:    [[TMP4:%.*]] = alloca [[STR]]
; CHECK:    store [[STR]] [[TMP3]], %str* [[TMP4]]
; CHECK:    [[TMP5:%.*]] = bitcast %str* [[TMP0:%.*]] to i8*
; CHECK:    [[TMP6:%.*]] = bitcast %str* [[TMP4]] to i8*
; CHECK:    call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 [[TMP5]], i8* align 8 [[TMP6]], i64 12, i1 false)
; CHECK:    ret void
;
  %1 = insertvalue %str { i32 42, i64 0 }, i32 %src1, 0
  %2 = insertvalue %str %1, i64 %src2, 1
  ret %str %2
}

declare void @use.i32(i32)
declare void @use.i64(i64)

attributes #0 = { "visaStackCall" }
attributes #1 = { "IndirectlyCalled" }
