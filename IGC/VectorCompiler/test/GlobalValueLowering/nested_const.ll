;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefix=CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefix=CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@array_a = internal global [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4
@array_b = internal global [8 x i32] [i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50], align 4
@array_c = internal global [8 x i32] [i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51], align 4

define dllexport void @simple_array() {
  %ptr.vec = alloca [3 x <2 x [8 x i32]*>]
  store [3 x <2 x [8 x i32]*>] [<2 x [8 x i32]*> <[8 x i32]* @array_a, [8 x i32]* @array_b>,
                                <2 x [8 x i32]*> <[8 x i32]* @array_b, [8 x i32]* @array_c>,
                                <2 x [8 x i32]*> <[8 x i32]* @array_c, [8 x i32]* @array_a>],
        [3 x <2 x [8 x i32]*>]* %ptr.vec

; COM: lower global variables
; CHECK-TYPED-PTRS-DAG: %array_a.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_a)
; CHECK-TYPED-PTRS-DAG: %array_a.lowered = inttoptr i64 %array_a.gaddr to [8 x i32]*
; CHECK-TYPED-PTRS-DAG: %array_b.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_b)
; CHECK-TYPED-PTRS-DAG: %array_b.lowered = inttoptr i64 %array_b.gaddr to [8 x i32]*
; CHECK-TYPED-PTRS-DAG: %array_c.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_c)
; CHECK-TYPED-PTRS-DAG: %array_c.lowered = inttoptr i64 %array_c.gaddr to [8 x i32]*
; CHECK-OPAQUE-PTRS-DAG: %array_a.gaddr = call i64 @llvm.genx.gaddr.i64.p0(ptr @array_a)
; CHECK-OPAQUE-PTRS-DAG: %array_a.lowered = inttoptr i64 %array_a.gaddr to ptr
; CHECK-OPAQUE-PTRS-DAG: %array_b.gaddr = call i64 @llvm.genx.gaddr.i64.p0(ptr @array_b)
; CHECK-OPAQUE-PTRS-DAG: %array_b.lowered = inttoptr i64 %array_b.gaddr to ptr
; CHECK-OPAQUE-PTRS-DAG: %array_c.gaddr = call i64 @llvm.genx.gaddr.i64.p0(ptr @array_c)
; CHECK-OPAQUE-PTRS-DAG: %array_c.lowered = inttoptr i64 %array_c.gaddr to ptr

; COM: lower vectors
; CHECK-TYPED-PTRS-DAG: %[[ZER_VEC_A:[^ ]+]] = insertelement <2 x [8 x i32]*> undef, [8 x i32]* %array_a.lowered, i64 0
; CHECK-TYPED-PTRS-DAG: %[[ZER_VEC:[^ ]+]] = insertelement <2 x [8 x i32]*> %[[ZER_VEC_A]], [8 x i32]* %array_b.lowered, i64 1
; CHECK-OPAQUE-PTRS-DAG: %[[ZER_VEC_A:[^ ]+]] = insertelement <2 x ptr> undef, ptr %array_a.lowered, i64 0
; CHECK-OPAQUE-PTRS-DAG: %[[ZER_VEC:[^ ]+]] = insertelement <2 x ptr> %[[ZER_VEC_A]], ptr %array_b.lowered, i64 1

; COM: this [[FST_VEC_A]] can be potentially avoided as [[ZER_VEC_A]] is the same
; CHECK-TYPED-PTRS-DAG: %[[FST_VEC_B:[^ ]+]] = insertelement <2 x [8 x i32]*> undef, [8 x i32]* %array_b.lowered, i64 0
; CHECK-TYPED-PTRS-DAG: %[[FST_VEC:[^ ]+]] = insertelement <2 x [8 x i32]*> %[[FST_VEC_B:[^ ]+]], [8 x i32]* %array_c.lowered, i64 1
; CHECK-OPAQUE-PTRS-DAG: %[[FST_VEC_B:[^ ]+]] = insertelement <2 x ptr> undef, ptr %array_b.lowered, i64 0
; CHECK-OPAQUE-PTRS-DAG: %[[FST_VEC:[^ ]+]] = insertelement <2 x ptr> %[[FST_VEC_B:[^ ]+]], ptr %array_c.lowered, i64 1

; CHECK-TYPED-PTRS-DAG: %[[SND_VEC_C:[^ ]+]] = insertelement <2 x [8 x i32]*> undef, [8 x i32]* %array_c.lowered, i64 0
; CHECK-TYPED-PTRS-DAG: %[[SND_VEC:[^ ]+]] = insertelement <2 x [8 x i32]*> %[[SND_VEC_C]], [8 x i32]* %array_a.lowered, i64 1
; CHECK-OPAQUE-PTRS-DAG: %[[SND_VEC_C:[^ ]+]] = insertelement <2 x ptr> undef, ptr %array_c.lowered, i64 0
; CHECK-OPAQUE-PTRS-DAG: %[[SND_VEC:[^ ]+]] = insertelement <2 x ptr> %[[SND_VEC_C]], ptr %array_a.lowered, i64 1

; COM: lower array
; CHECK-TYPED-PTRS-DAG: %[[ARR_ZER_VEC:[^ ]+]] = insertvalue [3 x <2 x [8 x i32]*>] undef, <2 x [8 x i32]*> %[[ZER_VEC]], 0
; CHECK-TYPED-PTRS-DAG: %[[ARR_FST_VEC:[^ ]+]] = insertvalue [3 x <2 x [8 x i32]*>] %[[ARR_ZER_VEC]], <2 x [8 x i32]*> %[[FST_VEC]], 1
; CHECK-TYPED-PTRS: %[[ARR:[^ ]+]] = insertvalue [3 x <2 x [8 x i32]*>] %[[ARR_FST_VEC]], <2 x [8 x i32]*> %[[SND_VEC]], 2
; CHECK-OPAQUE-PTRS-DAG: %[[ARR_ZER_VEC:[^ ]+]] = insertvalue [3 x <2 x ptr>] undef, <2 x ptr> %[[ZER_VEC]], 0
; CHECK-OPAQUE-PTRS-DAG: %[[ARR_FST_VEC:[^ ]+]] = insertvalue [3 x <2 x ptr>] %[[ARR_ZER_VEC]], <2 x ptr> %[[FST_VEC]], 1
; CHECK-OPAQUE-PTRS: %[[ARR:[^ ]+]] = insertvalue [3 x <2 x ptr>] %[[ARR_FST_VEC]], <2 x ptr> %[[SND_VEC]], 2

; CHECK-TYPED-PTRS: store [3 x <2 x [8 x i32]*>] %[[ARR]], [3 x <2 x [8 x i32]*>]* %ptr.vec
; CHECK-OPAQUE-PTRS: store [3 x <2 x ptr>] %[[ARR]], ptr %ptr.vec
  ret void
}
