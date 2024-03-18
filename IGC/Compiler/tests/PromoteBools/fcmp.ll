;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-promote-bools -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_kernel void @fcmp() {
  %C3 = fcmp oeq float zeroinitializer, zeroinitializer
  store i1 %C3, i1* null, align 1
  ret void
}

; CHECK-LABEL:  define spir_kernel void @fcmp()

; CHECK:        [[TMP0:%.*]] = fcmp oeq float
; CHECK-NEXT:   [[TMP1:%.*]] = zext i1 [[TMP0]] to i8
; CHECK-NEXT:   store i8 [[TMP1]], i8* null, align 1
