;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLoadStoreLowering -march=genx64 -mcpu=XeLPG -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLoadStoreLowering -march=genx64 -mcpu=XeLPG -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare void @llvm.genx.svm.block.st.i64.v8f32(i64, <8 x float>)

define void @test(float* %RET) {
  %p_foo.i = alloca [8 x <8 x i16>], align 16
  %svm_st_ptrtoint = ptrtoint float* %RET to i64
  tail call void @llvm.genx.svm.block.st.i64.v8f32(i64 %svm_st_ptrtoint, <8 x float> zeroinitializer)
  %pti = ptrtoint [8 x <8 x i16>]* %p_foo.i to i64
  %ipt = inttoptr i64 %pti to <8 x i16>*
  store <8 x i16> <i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8, i16 9>, <8 x i16>* %ipt

; CHECK-TYPED-PTRS: [[ADDR:%.*]] = ptrtoint <8 x i16>* %ipt to i64
; CHECK-OPAQUE-PTRS: [[ADDR:%.*]] = ptrtoint ptr %ipt to i64
; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v2i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 2, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR]], i16 1, i32 0, <2 x i64> bitcast (<8 x i16> <i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8, i16 9> to <2 x i64>))
  ret void
}
