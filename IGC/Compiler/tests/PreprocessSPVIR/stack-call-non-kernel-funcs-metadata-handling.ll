;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, debug
; RUN: igc_opt --opaque-pointers -igc-convert-user-semantic-decorator-on-functions -igc-process-func-attributes -S %s 2>&1 | FileCheck %s


@.str = internal unnamed_addr addrspace(2) constant [20 x i8] c"igc-force-stackcall\00"
@.str.1 = internal unnamed_addr addrspace(2) constant [2 x i8] c"1\00"
@gVar = private unnamed_addr constant [20 x i8] c"igc-force-stackcall\00", section "llvm.metadata"
@llvm.global.annotations = appending global [1 x { ptr, ptr, ptr, i32, ptr }] [{ ptr, ptr, ptr, i32, ptr } { ptr @Test, ptr @gVar, ptr undef, i32 undef, ptr undef }], section "llvm.metadata"

; CHECK-NOT: assertion failed
define spir_func i32 @Test(ptr addrspace(1) %ptr) {
entry:
  ret i32 1
}