;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-legalize-function-signatures -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------

; Test checks that arguments with int vector type are legalized

; Debug-info related checks
;
; For llvm 14 check-debugify treats missing debug location on argument truncation to legal type
; at the begining of BB as a warning, while on earlier llvm versions its treated as an error.
;
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_k(i32 %src) {
; CHECK-LABEL: @test_k(
; CHECK:    [[TMP1:%.*]] = zext i32 [[SRC:%.*]] to i48
; CHECK:    [[TMP2:%.*]] = insertelement <3 x i48> <i48 0, i48 42, i48 13>, i48 [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = zext <3 x i48> [[TMP2]] to <3 x i64>
; CHECK:    [[TMP4:%.*]] = call i32 @foo(<3 x i64> [[TMP3]])
; CHECK:    [[TMP5:%.*]] = alloca i32, align 4
; CHECK:    store i32 [[TMP4]], i32* [[TMP5]], align 4
; CHECK:    call void @bar(<3 x i48> [[TMP2]])
; CHECK:    ret void
;
  %1 = zext i32 %src to i48
  %2 = insertelement <3 x i48> <i48 0, i48 42, i48 13>, i48 %1, i32 0
  %3 = call i32 @foo(<3 x i48> %2)
  %4 = alloca i32, align 4
  store i32 %3, i32* %4, align 4
  call void @bar(<3 x i48> %2)
  ret void
}

define spir_func i32 @foo(<3 x i48> %src) {
; CHECK-LABEL: define spir_func i32 @foo(<3 x i64> %src)
; CHECK:    [[TMP1:%.*]] = trunc <3 x i64> [[SRC:%.*]] to <3 x i48>
; CHECK:    [[TMP2:%.*]] = alloca <3 x i48>, align 32
; CHECK:    store <3 x i48> [[TMP1]], <3 x i48>* [[TMP2]], align 32
; CHECK:    [[TMP3:%.*]] = load <3 x i48>, <3 x i48>* [[TMP2]], align 32
; CHECK:    [[TMP4:%.*]] = extractelement <3 x i48> [[TMP3]], i32 0
; CHECK:    [[TMP5:%.*]] = extractelement <3 x i48> [[TMP1]], i32 1
; CHECK:    [[TMP6:%.*]] = add i48 [[TMP4]], [[TMP5]]
; CHECK:    [[TMP7:%.*]] = trunc i48 [[TMP6]] to i32
; CHECK:    ret i32 [[TMP7]]
;
  %1 = alloca <3 x i48>, align 32
  store <3 x i48> %src, <3 x i48>* %1, align 32
  %2 = load <3 x i48>, <3 x i48>* %1, align 32
  %3 = extractelement <3 x i48> %2, i32 0
  %4 = extractelement <3 x i48> %src, i32 1
  %5 = add i48 %3, %4
  %6 = trunc i48 %5 to i32
  ret i32 %6
}

declare void @bar(<3 x i48>) #0


attributes #0 = { "IndirectlyCalled" }
