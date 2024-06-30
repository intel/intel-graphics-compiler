;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s

declare float @llvm.vc.internal.atomic.fmin.f32.p3f32.f32(float addrspace(3)*, i32, i32, float) #0
declare float @llvm.vc.internal.atomic.fmax.f32.p3f32.f32(float addrspace(3)*, i32, i32, float) #0

define float @fmin_float(float addrspace(3)* %ptr, float %arg) {
  ; CHECK: [[FMIN_ADDR:%[^ ]+]] = ptrtoint float addrspace(3)* %ptr to i32
  ; CHECK: [[FMIN_VADDR:%[^ ]+]] = bitcast i32 [[FMIN_ADDR]] to <1 x i32>
  ; CHECK: [[FMIN_VDATA:%[^ ]+]] = bitcast float %arg to <1 x float>
  ; CHECK: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK: [[FMIN_RESULT:%[^ ]+]] = call <1 x float> @llvm.vc.internal.lsc.atomic.slm.v1f32.v1i1.v2i8.v1i32(<1 x i1> <i1 true>, i8 21, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <1 x i32> [[FMIN_VADDR]], i16 1, i32 0, <1 x float> [[FMIN_VDATA]], <1 x float> undef, <1 x float> undef)
  %res = call float @llvm.vc.internal.atomic.fmin.f32.p3f32.f32(float addrspace(3)* %ptr, i32 3, i32 16, float %arg) ; "Subgroup", "SequentiallyConsistent"
  ; CHECK: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 0)
  ; CHECK: %res = bitcast <1 x float> [[FMIN_RESULT]] to float
  ret float %res
}

define float @fmax_float(float addrspace(3)* %ptr, float %arg) {
  ; CHECK: [[FMAX_ADDR:%[^ ]+]] = ptrtoint float addrspace(3)* %ptr to i32
  ; CHECK: [[FMAX_VADDR:%[^ ]+]] = bitcast i32 [[FMAX_ADDR]] to <1 x i32>
  ; CHECK: [[FMAX_VDATA:%[^ ]+]] = bitcast float %arg to <1 x float>
  ; CHECK: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 1)
  ; CHECK: [[FMAX_RESULT:%[^ ]+]] = call <1 x float> @llvm.vc.internal.lsc.atomic.slm.v1f32.v1i1.v2i8.v1i32(<1 x i1> <i1 true>, i8 22, i8 2, i8 3, <2 x i8> zeroinitializer, i32 0, <1 x i32> [[FMAX_VADDR]], i16 1, i32 0, <1 x float> [[FMAX_VDATA]], <1 x float> undef, <1 x float> undef)
  %res = call float @llvm.vc.internal.atomic.fmax.f32.p3f32.f32(float addrspace(3)* %ptr, i32 2, i32 256, float %arg) ; "Workgroup", "WorkgroupMemory"
  ; CHECK: call void @llvm.genx.lsc.fence.i1(i1 true, i8 3, i8 0, i8 1)
  ; CHECK: %res = bitcast <1 x float> [[FMAX_RESULT]] to float
  ret float %res
}

attributes #0 = { nounwind }
