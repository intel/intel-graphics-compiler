;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-LSC,CHECK-LSC-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-LSC,CHECK-LSC-OPAQUE-PTRS

define i16 @inc_i16(i16 addrspace(1)* %ptr) {
  ; CHECK-LSC-TYPED-PTRS: [[INC_ADDR:%[^ ]+]] = ptrtoint i16 addrspace(1)* %ptr to i64
  ; CHECK-LSC-OPAQUE-PTRS: [[INC_ADDR:%[^ ]+]] = ptrtoint ptr addrspace(1) %ptr to i64
  ; CHECK-LSC: [[INC_VADDR:%[^ ]+]] = bitcast i64 [[INC_ADDR]] to <1 x i64>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 0, i8 0)
  ; CHECK-LSC: [[INC_VRES:%[^ ]+]] = call <1 x i32> @llvm.vc.internal.lsc.atomic.ugm.v1i32.v1i1.v2i8.v1i64(<1 x i1> <i1 true>, i8 8, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, <1 x i64> [[INC_VADDR]], i16 1, i32 0, <1 x i32> undef, <1 x i32> undef, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 0, i8 0)
  ; CHECK-LSC: [[INC_BCAST:%[^ ]+]] = bitcast <1 x i32> [[INC_VRES]] to i32
  ; CHECK-LSC: %res = trunc i32 [[INC_BCAST]] to i16
  %res = atomicrmw add i16 addrspace(1)* %ptr, i16 1 syncscope("subgroup") seq_cst
  ret i16 %res
}

define i16 @dec_i16(i16 addrspace(1)* %ptr) {
  ; CHECK-LSC-TYPED-PTRS: [[DEC_ADDR:%[^ ]+]] = ptrtoint i16 addrspace(1)* %ptr to i64
  ; CHECK-LSC-OPAQUE-PTRS: [[DEC_ADDR:%[^ ]+]] = ptrtoint ptr addrspace(1) %ptr to i64
  ; CHECK-LSC: [[DEC_VADDR:%[^ ]+]] = bitcast i64 [[DEC_ADDR]] to <1 x i64>
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 0, i8 0)
  ; CHECK-LSC: [[DEC_VRES:%[^ ]+]] = call <1 x i32> @llvm.vc.internal.lsc.atomic.ugm.v1i32.v1i1.v2i8.v1i64(<1 x i1> <i1 true>, i8 9, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, <1 x i64> [[DEC_VADDR]], i16 1, i32 0, <1 x i32> undef, <1 x i32> undef, <1 x i32> undef)
  ; CHECK-LSC: call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 0, i8 0)
  ; CHECK-LSC: [[DEC_BCAST:%[^ ]+]] = bitcast <1 x i32> [[DEC_VRES]] to i32
  ; CHECK-LSC: %res = trunc i32 [[DEC_BCAST]] to i16
  %res = atomicrmw sub i16 addrspace(1)* %ptr, i16 1 syncscope("workgroup") seq_cst
  ret i16 %res
}
