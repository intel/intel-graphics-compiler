;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

@0 = private unnamed_addr constant [9 x i8] zeroinitializer, section "llvm.metadata"
@llvm.global.annotations = appending global [1 x { i8*, i8*, i8*, i32 }] [{ i8*, i8*, i8*, i32 } { i8* bitcast (i8 (i1)* @foo to i8*), i8* getelementptr inbounds ([9 x i8], [9 x i8]* @0, i32 0, i32 0), i8* undef, i32 undef }], section "llvm.metadata"

define spir_func i8 @foo(i1 %input) {
  %1 = zext i1 %input to i8
  ret i8 %1
}

; CHECK:        @llvm.global.annotations = appending global [1 x { i8*, i8*, i8*, i32 }] [{ i8*, i8*, i8*, i32 } { i8* bitcast (i8 (i8)* @foo to i8*), i8* getelementptr inbounds ([9 x i8], [9 x i8]* @0, i32 0, i32 0), i8* undef, i32 undef }], section "llvm.metadata"

; CHECK-LABEL:  define spir_func i8 @foo(i8 %input)
; CHECK-NEXT:   %1 = trunc i8 %input to i1
; CHECK-NEXT:   %2 = zext i1 %1 to i8
; CHECK-NEXT:   ret i8 %2
