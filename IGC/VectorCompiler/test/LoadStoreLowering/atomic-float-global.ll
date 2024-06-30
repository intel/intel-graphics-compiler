;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s

declare float @llvm.vc.internal.atomic.fmin.f32.p1f32.f32(float addrspace(1)*, i32, i32, float) #0
declare float @llvm.vc.internal.atomic.fmax.f32.p1f32.f32(float addrspace(1)*, i32, i32, float) #0

define float @fmin_float(float addrspace(1)* %ptr, float %arg) {
  ; CHECK: [[FMIN_ADDR:%[^ ]+]] = ptrtoint float addrspace(1)* %ptr to i64
  ; CHECK: [[FMIN_VADDR:%[^ ]+]] = bitcast i64 [[FMIN_ADDR]] to <1 x i64>
  ; CHECK: [[FMIN_VDATA:%[^ ]+]] = bitcast float %arg to <1 x float>
  ; CHECK: call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 0, i8 2)
  ; CHECK: [[FMIN_RES:%[^ ]]] = call <1 x float> @llvm.vc.internal.lsc.atomic.ugm.v1f32.v1i1.v2i8.v1i64(<1 x i1> <i1 true>, i8 21, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <1 x i64> [[FMIN_VADDR]], i16 1, i32 0, <1 x float> [[FMIN_VDATA]], <1 x float> undef, <1 x float> undef)
  %res = call float @llvm.vc.internal.atomic.fmin.f32.p1f32.f32(float addrspace(1)* %ptr, i32 1, i32 8, float %arg) ; "Device", "AcquireRelease"
  ; CHECK: call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 0, i8 2)
  ; CHECK: %res = bitcast <1 x float> [[FMIN_RES]] to float
  ret float %res
}

define float @fmax_float(float addrspace(1)* %ptr, float %arg) {
  ; CHECK: [[FMAX_ADDR:%[^ ]+]] = ptrtoint float addrspace(1)* %ptr to i64
  ; CHECK: [[FMAX_VADDR:%[^ ]+]] = bitcast i64 [[FMAX_ADDR]] to <1 x i64>
  ; CHECK: [[FMAX_VDATA:%[^ ]+]] = bitcast float %arg to <1 x float>
  ; CHECK-NOT: call void @llvm.genx.lsc.fence
  ; CHECK: [[FMAX_RES:%[^ ]]] = call <1 x float> @llvm.vc.internal.lsc.atomic.ugm.v1f32.v1i1.v2i8.v1i64(<1 x i1> <i1 true>, i8 22, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <1 x i64> [[FMAX_VADDR]], i16 1, i32 0, <1 x float> [[FMAX_VDATA]], <1 x float> undef, <1 x float> undef)
  %res = call float @llvm.vc.internal.atomic.fmax.f32.p1f32.f32(float addrspace(1)* %ptr, i32 0, i32 0, float %arg) ; "CrossDevice", "Relaxed"
  ; CHECK: %res = bitcast <1 x float> [[FMAX_RES]] to float
  ret float %res
}

attributes #0 = { nounwind }
