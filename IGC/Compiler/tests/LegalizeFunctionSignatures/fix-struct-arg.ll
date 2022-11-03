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

; Test checks that struct argument is legalized with byval attribute

; Debug-info related checks
;
; For llvm 14 check-debugify treats missing debug location on argument load at the begining of BB
; as a warning, while on earlier llvm versions its treated as an error.
;
; CHECK-LLVM-14-PLUS: CheckModuleDebugify: PASS

%str = type { i32, i64 }

define spir_kernel void @test_k(i32 %src) {
; CHECK-LABEL: @test_k(
; CHECK:    [[TMP1:%[A-z0-9]*]] = insertvalue [[STR:%[A-z0-9]*]] { i32 0, i64 32 }, i32 [[SRC:%[A-z0-9]*]], 0
; CHECK:    [[TMP2:%[A-z0-9]*]] = alloca [[STR]]
; CHECK:    store [[STR]] [[TMP1]], %str* [[TMP2]]
; CHECK:    [[TMP3:%[A-z0-9]*]] = call i32 @foo(%str* byval([[STR]]) [[TMP2]])
; CHECK:    [[TMP4:%[A-z0-9]*]] = insertvalue [2 x i32] [i32 0, i32 32], i32 [[SRC]], 0
; CHECK:    [[TMP5:%[A-z0-9]*]] = insertvalue [2 x i32] [[TMP4]], i32 [[TMP3]], 1
; CHECK:    call void @bar([2 x i32] [[TMP4]])
; CHECK:    ret void
;
  %1 = insertvalue %str { i32 0, i64 32 }, i32 %src, 0
  %2 = call i32 @foo(%str %1)
  %3 = insertvalue [2 x i32] [i32 0, i32 32], i32 %src, 0
  %4 = insertvalue [2 x i32] %3, i32 %2, 1
  call void @bar([2 x i32] %3)
  ret void
}

define spir_func i32 @foo(%str %src) #0 {
; CHECK-LABEL: define spir_func i32 @foo(%str* byval(%str) %src)
; CHECK:    [[TMP1:%[A-z0-9]*]] = load [[STR:%[A-z0-9]*]], %str* [[SRC:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = extractvalue [[STR]] [[TMP1]], 0
; CHECK:    ret i32 [[TMP2]]
;
  %1 = extractvalue %str %src, 0
  ret i32 %1
}

declare void @bar([2 x i32]) #1

attributes #0 = { "visaStackCall" }
attributes #1 = { "IndirectlyCalled" }
