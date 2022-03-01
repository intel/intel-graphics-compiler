;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -resolve-pointers-comparison | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define internal spir_func void @func(i32 addrspace(3)* %ptr0, i32 addrspace(3)* %ptr1) {
  %op0 = ptrtoint i32 addrspace(3)* %ptr0 to i64
  %op1 = ptrtoint i32 addrspace(3)* %ptr1 to i64

  ; CHECK: icmp eq i32 addrspace(3)* %ptr0, %ptr1
  %equal = icmp eq i64 %op0, %op1

  ; CHECK: icmp ne i32 addrspace(3)* %ptr0, %ptr1
  %not_equal = icmp ne i64 %op0, %op1

  ; CHECK: icmp ugt i32 addrspace(3)* %ptr0, %ptr1
  %greater_than = icmp ugt i64 %op0, %op1

  ; CHECK: icmp ult i32 addrspace(3)* %ptr0, %ptr1
  %lower_than = icmp ult i64 %op0, %op1

  ; CHECK: icmp uge i32 addrspace(3)* %ptr0, %ptr1
  %greater_than_or_equal = icmp uge i64 %op0, %op1

  ; CHECK: icmp ule i32 addrspace(3)* %ptr0, %ptr1
  %lower_than_or_equal = icmp ule i64 %op0, %op1

  ret void
}
