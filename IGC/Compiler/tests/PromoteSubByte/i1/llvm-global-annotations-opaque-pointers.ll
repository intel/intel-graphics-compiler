;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-promote-sub-byte -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

@0 = private unnamed_addr constant [9 x i8] zeroinitializer, section "llvm.metadata"
@llvm.global.annotations = appending global [1 x { ptr, ptr, ptr, i32 }] [{ ptr, ptr, ptr, i32 } { ptr @foo, ptr @0, ptr undef, i32 undef }], section "llvm.metadata"

define spir_func i8 @foo(i1 %input) {
  %1 = zext i1 %input to i8
  ret i8 %1
}

; CHECK: @llvm.global.annotations = appending global [1 x { ptr, ptr, ptr, i32 }] [{ ptr, ptr, ptr, i32 } { ptr @foo, ptr @0, ptr undef, i32 undef }], section "llvm.metadata"

; CHECK-LABEL:  define spir_func i8 @foo(i8 %input)
; CHECK-NEXT:   %1 = trunc i8 %input to i1
; CHECK-NEXT:   %2 = zext i1 %1 to i8
; CHECK-NEXT:   ret i8 %2
