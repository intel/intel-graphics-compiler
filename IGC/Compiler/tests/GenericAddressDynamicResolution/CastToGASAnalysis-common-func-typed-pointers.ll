;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-generic-address-dynamic-resolution | FileCheck %s

; This test verifies the case where a function which operates on a generic
; pointer is called from two kernels.

;    kernelA           kernelB
;           \         /
;            \       /
;            f_common

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"

define spir_func void @f_common(i32 addrspace(4)* %generic_ptr) {
  store i32 5, i32 addrspace(4)* %generic_ptr, align 4
  ; CHECK: %[[PTI:.*]] = ptrtoint i32 addrspace(4)* %generic_ptr to i64
  ; CHECK: %[[TAG:.*]] = lshr i64 %1, 61
  ; switch i64 %[[TAG]], label %GlobalBlock [
  ;  i64 2, label %LocalBlock
  ; ]

  ; LocalBlock:
  ; %[[LOCAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(3)*
  ; store i32 5, i32 addrspace(3)* %[[LOCAL_PTR]], align 4

  ; GlobalBlock:
  ; %[[GLOBAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(1)*
  ; store i32 5, i32 addrspace(1)* %[[GLOBAL_PTR]], align 4
  ret void
}

define spir_kernel void @kernelA(i32 addrspace(1)* %global_buffer) {
  %generic_ptr = addrspacecast i32 addrspace(1)* %global_buffer to i32 addrspace(4)*
  call spir_func void @f_common(i32 addrspace(4)* %generic_ptr)
  ret void
}

define spir_kernel void @kernelB(i32 addrspace(3)* %local_buffer) {
  %generic_ptr = addrspacecast i32 addrspace(3)* %local_buffer to i32 addrspace(4)*
  call spir_func void @f_common(i32 addrspace(4)* %generic_ptr)
  ret void
}
