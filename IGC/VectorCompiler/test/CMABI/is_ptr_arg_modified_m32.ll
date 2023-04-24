;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx32 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:32:32-i64:64-n8:16:32"

declare void @llvm.genx.scatter.scaled.v1i1.v1i32.v1f64(<1 x i1>, i32, i16, i32, i32, <1 x i32>, <1 x double>)
declare <1 x double> @llvm.genx.gather.scaled.v1f64.v1i1.v1i32(<1 x i1>, i32, i16, i32, i32, <1 x i32>, <1 x double>)

define internal spir_func void @use_scatter_scaled(<1 x double>* %vector.ptr) {
  %offset = ptrtoint <1 x double>* %vector.ptr to i32
  call void @llvm.genx.scatter.scaled.v1i1.v1i32.v1f64(<1 x i1> <i1 true>, i32 3, i16 0, i32 255, i32 %offset, <1 x i32> <i32 0>, <1 x double> zeroinitializer)
  ret void
}

define internal spir_func void @use_gather_scaled(<1 x double>* %vector.ptr) {
  %offset = ptrtoint <1 x double>* %vector.ptr to i32
  %res = call <1 x double> @llvm.genx.gather.scaled.v1f64.v1i1.v1i32(<1 x i1> <i1 true>, i32 3, i16 0, i32 255, i32 %offset, <1 x i32> <i32 0>, <1 x double> undef)
  ret void
}

define dllexport void @kernel() {
  %vec1.mem = alloca <1 x double>, align 32
  call spir_func void @use_scatter_scaled(<1 x double>* %vec1.mem)
; CHECK: %[[RET:[^ ]+]] = call spir_func <1 x double> @use_scatter_scaled(<1 x double> %vec1.mem.val)
; CHECK: store <1 x double> %[[RET]], <1 x double>* %vec1.mem

  %vec2.mem = alloca <1 x double>, align 32
  call spir_func void @use_gather_scaled(<1 x double>* %vec2.mem)
; CHECK: call spir_func void @use_gather_scaled(<1 x double> %vec2.mem.val)

  ret void
}

!genx.kernels = !{!0}
!0 = !{void ()* @kernel}
