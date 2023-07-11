;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; CHECK: define internal spir_func void @_Z6do_addPfS_S_
; CHECK-SAME: float addrspace(4)*
; CHECK-SAME: float addrspace(4)*

; CHECK-NOT: alloca

; Function Attrs: nounwind readonly
declare <32 x float> @llvm.genx.svm.block.ld.unaligned.v32f32.i64(i64) #0

; Function Attrs: nounwind
declare void @llvm.genx.svm.block.st.i64.v32f32(i64, <32 x float>) #1

; Function Attrs: noinline nounwind
define internal spir_func void @_Z6do_addPfS_S_(float addrspace(4)* %A, float addrspace(4)* %C) #3 {
entry:
  %0 = ptrtoint float addrspace(4)* %A to i64
  %call.i.esimd = call <32 x float> @llvm.genx.svm.block.ld.unaligned.v32f32.i64(i64 %0)
  %add.i = fadd <32 x float> %call.i.esimd, zeroinitializer
  %1 = ptrtoint float addrspace(4)* %C to i64
  call void @llvm.genx.svm.block.st.i64.v32f32(i64 %1, <32 x float> %add.i)
  ret void
}

; Function Attrs: nounwind
define dllexport void @_ZTSZZ4mainENKUlRN2cl4sycl7handlerEE_clES2_E4Test(float addrspace(1)* %__arg_1, float addrspace(1)* %__arg_2) #4 {
entry:
  %add.ptr4.i = addrspacecast float addrspace(1)* %__arg_1 to float addrspace(4)*
  %add.ptr7.i = addrspacecast float addrspace(1)* %__arg_2 to float addrspace(4)*
  call spir_func void @_Z6do_addPfS_S_(float addrspace(4)* %add.ptr4.i, float addrspace(4)* %add.ptr7.i) #5
  ret void
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }
attributes #3 = { noinline nounwind }
attributes #4 = { nounwind "CMGenxMain" "oclrt"="1" }
attributes #5 = { noinline nounwind }

!genx.kernels = !{!0}

!0 = !{void (float addrspace(1)*, float addrspace(1)*)* @_ZTSZZ4mainENKUlRN2cl4sycl7handlerEE_clES2_E4Test, !"_ZTSZZ4mainENKUlRN2cl4sycl7handlerEE_clES2_E4Test", i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}

