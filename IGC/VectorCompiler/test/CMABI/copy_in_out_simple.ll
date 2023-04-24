;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; CHECK: define internal spir_func float @_Z6do_addPfS_S_(float [[ARG1:%[0-9a-zA-Z]+]], float [[ARG2:%[0-9a-zA-Z]+]])
; CHECK: [[ALLOCA1:%[0-9a-zA-Z]+]] = alloca

; CHECK: store
; CHECK-SAME: [[ARG1]]
; CHECK-SAME: [[ALLOCA1]]

; CHECK: [[ALLOCA2:%[0-9a-zA-Z]+]] = alloca

; CHECK: store
; CHECK-SAME: [[ARG2]]
; CHECK-SAME: [[ALLOCA2]]

; CHECK: load
; CHECK: store
; CHECK: [[RES:%[0-9a-zA-Z]+]] = load float, float* [[ALLOCA2]]

; CHECK: ret
; CHECK-SAME: [[RES]]

; Function Attrs: noinline nounwind
define internal spir_func void @_Z6do_addPfS_S_(float addrspace(4)* %A, float addrspace(4)* %C) #3 {
entry:
  %val = load float, float addrspace(4)* %A
  %add.i = fadd float %val, zeroinitializer
  store float %add.i, float addrspace(4)* %C
  ret void
}

; CHECK: define dllexport void @_ZTSZZ4mainENKUlRN2cl4sycl7handlerEE_clES2_E4Test
; CHECK: [[INARG1:%[0-9a-zA-Z.]+]] = load
; CHECK: [[INARG2:%[0-9a-zA-Z.]+]] = load
; CHECK-SAME: [[PTR2:%[0-9a-zA-Z.]+]]

; CHECK: [[CALL:%[0-9a-zA-Z]+]] = call
; CHECK-SAME: _Z6do_addPfS_S_
; CHECK-SAME: [[INARG1]]
; CHECK-SAME: [[INARG2]]

; CHECK: store
; CHECK-SAME: [[CALL]]
; CHECK-SAME: [[PTR2]]

; Function Attrs: nounwind
define dllexport void @_ZTSZZ4mainENKUlRN2cl4sycl7handlerEE_clES2_E4Test(float addrspace(1)* %__arg_1, float addrspace(1)* %__arg_2) #4 {
entry:
  %arg1 = addrspacecast float addrspace(1)* %__arg_1 to float addrspace(4)*
  %arg2 = addrspacecast float addrspace(1)* %__arg_2 to float addrspace(4)*
  call spir_func void @_Z6do_addPfS_S_(float addrspace(4)* %arg1, float addrspace(4)* %arg2) #5
  ret void
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }
attributes #3 = { noinline nounwind }
attributes #4 = { nounwind "CMGenxMain" "oclrt"="1" }
attributes #5 = { noinline nounwind }

!genx.kernels = !{!0}

!0 = !{void (float addrspace(1)*, float addrspace(1)*)* @_ZTSZZ4mainENKUlRN2cl4sycl7handlerEE_clES2_E4Test, !"_ZTSZZ4mainENKUlRN2cl4sycl7handlerEE_clES2_E4Test", i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}

