;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-gas-resolve | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define internal spir_func void @test_local(i32 addrspace(3)* %src0, i32 addrspace(3)* %src1, i32 addrspace(4)* %unknown_generic_ptr) {
  %src0_as_generic = addrspacecast i32 addrspace(3)* %src0 to i32 addrspace(4)*
  %src1_as_generic = addrspacecast i32 addrspace(3)* %src1 to i32 addrspace(4)*

  ; BASIC TEST CASE
  ; CHECK: %basic = select i1 false, i32 addrspace(3)* %src0, i32 addrspace(3)* %src1
  ; CHECK: store i32 5, i32 addrspace(3)* %basic, align 4
  %basic = select i1 false, i32 addrspace(4)* %src0_as_generic, i32 addrspace(4)* %src1_as_generic
  store i32 5, i32 addrspace(4)* %basic, align 4

  ; NEGATIVE TEST CASE (expecting not lowered select instruction))
  ; CHECK-NOT: select i1 true, i32 addrspace(3)* %{{.*}}, i32 addrspace(3)* %{{.*}}
  %negative = select i1 true, i32 addrspace(4)* %src0_as_generic, i32 addrspace(4)* %unknown_generic_ptr
  store i32 5, i32 addrspace(4)* %negative, align 4

  ; ONE-SELECT-ARGUMENT-IS-NULL TEST CASE
  ; CHECK: %null_operand = select i1 true, i32 addrspace(3)* %src0, i32 addrspace(3)* null
  ; CHECK: store i32 5, i32 addrspace(3)* %null_operand, align 4
  %null_operand = select i1 true, i32 addrspace(4)* %src0_as_generic, i32 addrspace(4)* null
  store i32 5, i32 addrspace(4)* %null_operand, align 4

  ret void
}

