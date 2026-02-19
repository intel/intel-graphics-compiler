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

define spir_kernel void @icmps() {
  %C = icmp eq i32 0, 0
  store i1 %C, i1* null, align 1
  ret void
}

; CHECK-LABEL:  define spir_kernel void @icmps()

; CHECK:        [[CMP:%[0-9]+]] = icmp eq i32 0, 0
; CHECK-NEXT:   [[EXT:%[0-9]+]] = zext i1 [[CMP]] to i8
; CHECK-NEXT:   store i8 [[EXT]], i8* null, align 1
