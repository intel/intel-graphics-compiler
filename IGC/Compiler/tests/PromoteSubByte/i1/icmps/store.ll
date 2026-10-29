;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -igc-promote-sub-byte -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -igc-promote-sub-byte -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

define spir_kernel void @icmps() {
  %C = icmp eq i32 0, 0
  store i1 %C, i1* null, align 1
  ret void
}

; CHECK-LABEL:  define spir_kernel void @icmps()

; CHECK:        [[CMP:%[0-9]+]] = icmp eq i32 0, 0
; CHECK-NEXT:   [[EXT:%[0-9]+]] = zext i1 [[CMP]] to i8
; CHECK-TYPED-NEXT:   store i8 [[EXT]], i8* null, align 1
; CHECK-OPAQUE-NEXT:   store i8 [[EXT]], ptr null, align 1
