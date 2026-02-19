;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-generic-address-dynamic-resolution | FileCheck %s

; This test verifies whether optimization which allows to avoid additional control flow
; generation works per kernel. Below LLVM module implements two kernels:
; - kernelA - contains store instruction operating on generic pointer which may point
;             to either private or local memory depending on global_id
; - kernelB - contains store instruction operating on generic pointer which always
;             point to global memory

; Since kernelB always writes data to global memory, there is no need to generate
; additional control flow to handle generic pointer. GenericAddressDynamicResolution pass
; uses CastToGASAnalysis to decide whether control flow generation can be avoided for
; a particular kernel. The purpose of including two kernels in this LLVM module is to
; verify whether addrspacecast's from kernelA don't block applying the optimization
; on kernelB.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"

define spir_kernel void @kernelA(i32 addrspace(3)* %local_buffer) {
  %alloca = alloca i32
  %call = call spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()
  %gid = extractelement <3 x i64> %call, i32 0
  %cond = icmp ne i64 %gid, 0
  br i1 %cond, label %br0, label %br1
br0:
  %private_as_generic = addrspacecast i32* %alloca to i32 addrspace(4)*
  br label %return
br1:
  %local_as_generic = addrspacecast i32 addrspace(3)* %local_buffer to i32 addrspace(4)*
  br label %return
return:
  %generic_ptr = phi i32 addrspace(4)* [ %private_as_generic, %br0 ], [ %local_as_generic, %br1 ]
  ; CHECK: %[[PTI:.*]] = ptrtoint i32 addrspace(4)* %generic_ptr to i64
  ; CHECK: %[[TAG:.*]] = lshr i64 %[[PTI]], 61
  ; CHECK: switch i64 %[[TAG]], label %GlobalBlock [
  ; CHECK:   i64 2, label %LocalBlock
  ; CHECK: ]

  ; CHECK: LocalBlock:
  ; CHECK:   %[[LOCAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(3)*
  ; CHECK:   store i32 5, i32 addrspace(3)* %[[LOCAL_PTR]], align 4

  ; CHECK: GlobalBlock:
  ; CHECK:   %[[GLOBAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(1)*
  ; CHECK:   store i32 5, i32 addrspace(1)* %[[GLOBAL_PTR]], align 4

  store i32 5, i32 addrspace(4)* %generic_ptr, align 4
  ret void
}

define spir_kernel void @kernelB(i32 addrspace(1)* %global_buffer) {
  %generic_ptr = addrspacecast i32 addrspace(1)* %global_buffer to i32 addrspace(4)*
  ; CHECK: %[[GLOBAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(1)*
  ; CHECK: store i32 5, i32 addrspace(1)* %[[GLOBAL_PTR]], align 4
  store i32 5, i32 addrspace(4)* %generic_ptr, align 4
  ret void
}

declare spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()
