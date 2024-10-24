;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@array_a = internal global [8 x i32] [i32 42, i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49], align 4
@array_b = internal global [8 x i32] [i32 43, i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50], align 4
@array_c = internal global [8 x i32] [i32 44, i32 45, i32 46, i32 47, i32 48, i32 49, i32 50, i32 51], align 4

%struct.inner = type { [8 x i32]*, i32* }
%struct.outer = type { i32, i32*, %struct.inner, [2 x [8 x i32]*], <2 x [8 x i32]*> }
define dllexport void @simple_array() {
  %ptr.vec = alloca %struct.outer
  store %struct.outer { i32 23, i32* getelementptr inbounds ([8 x i32], [8 x i32]* @array_a, i32 0, i64 3),
                        %struct.inner { [8 x i32]* @array_a,
                                        i32* getelementptr inbounds ([8 x i32], [8 x i32]* @array_a, i32 0, i64 5) },
                        [2 x [8 x i32]*] [[8 x i32]* @array_b, [8 x i32]* @array_c],
                        <2 x [8 x i32]*> <[8 x i32]* @array_c, [8 x i32]* @array_a> },
        %struct.outer* %ptr.vec

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

; COM: lower GEP
; CHECK-TYPED-PTRS-DAG: %[[GEP:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %array_a.lowered, i32 0, i64 3
; CHECK-OPAQUE-PTRS-DAG: %[[GEP:[^ ]+]] = getelementptr inbounds [8 x i32], ptr %array_a.lowered, i32 0, i64 3

; COM: lower inner struct
; CHECK-TYPED-PTRS-DAG: %[[INN_ST_HALF:[^ ]+]] = insertvalue %struct.inner undef, [8 x i32]* %array_a.lowered, 0
; CHECK-TYPED-PTRS-DAG: %[[INN_ST_GEP:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %array_a.lowered, i32 0, i64 5
; CHECK-TYPED-PTRS-DAG: %[[INN_ST:[^ ]+]] = insertvalue %struct.inner %[[INN_ST_HALF]], i32* %[[INN_ST_GEP]], 1
; CHECK-OPAQUE-PTRS-DAG: %[[INN_ST_HALF:[^ ]+]] = insertvalue %struct.inner undef, ptr %array_a.lowered, 0
; CHECK-OPAQUE-PTRS-DAG: %[[INN_ST_GEP:[^ ]+]] = getelementptr inbounds [8 x i32], ptr %array_a.lowered, i32 0, i64 5
; CHECK-OPAQUE-PTRS-DAG: %[[INN_ST:[^ ]+]] = insertvalue %struct.inner %[[INN_ST_HALF]], ptr %[[INN_ST_GEP]], 1

; COM: lower array
; CHECK-TYPED-PTRS-DAG: %[[ARR_HALF:[^ ]+]] = insertvalue [2 x [8 x i32]*] undef, [8 x i32]* %array_b.lowered, 0
; CHECK-TYPED-PTRS-DAG: %[[ARR:[^ ]+]] = insertvalue [2 x [8 x i32]*] %[[ARR_HALF]], [8 x i32]* %array_c.lowered, 1
; CHECK-OPAQUE-PTRS-DAG: %[[ARR_HALF:[^ ]+]] = insertvalue [2 x ptr] undef, ptr %array_b.lowered, 0
; CHECK-OPAQUE-PTRS-DAG: %[[ARR:[^ ]+]] = insertvalue [2 x ptr] %[[ARR_HALF]], ptr %array_c.lowered, 1

; COM: lower vector
; CHECK-TYPED-PTRS-DAG: %[[VEC_HALF:[^ ]+]] = insertelement <2 x [8 x i32]*> undef, [8 x i32]* %array_c.lowered, i64 0
; CHECK-TYPED-PTRS-DAG: %[[VEC:[^ ]+]] = insertelement <2 x [8 x i32]*> %[[VEC_HALF]], [8 x i32]* %array_a.lowered, i64 1
; CHECK-OPAQUE-PTRS-DAG: %[[VEC_HALF:[^ ]+]] = insertelement <2 x ptr> undef, ptr %array_c.lowered, i64 0
; CHECK-OPAQUE-PTRS-DAG: %[[VEC:[^ ]+]] = insertelement <2 x ptr> %[[VEC_HALF]], ptr %array_a.lowered, i64 1

; COM: lower outer struct
; CHECK-TYPED-PTRS-DAG: %[[OUT_ST_GEP:[^ ]+]] = insertvalue %struct.outer { i32 23, i32* undef, %struct.inner undef, [2 x [8 x i32]*] undef, <2 x [8 x i32]*> undef }, i32* %[[GEP]], 1
; CHECK-OPAQUE-PTRS-DAG: %[[OUT_ST_GEP:[^ ]+]] = insertvalue %struct.outer { i32 23, ptr undef, %struct.inner undef, [2 x ptr] undef, <2 x ptr> undef }, ptr %[[GEP]], 1
; CHECK-DAG: %[[OUT_ST_INN_ST:[^ ]+]] = insertvalue %struct.outer %[[OUT_ST_GEP]], %struct.inner %[[INN_ST]], 2
; CHECK-TYPED-PTRS-DAG: %[[OUT_ST_ARR:[^ ]+]] = insertvalue %struct.outer %[[OUT_ST_INN_ST]], [2 x [8 x i32]*] %[[ARR]], 3
; CHECK-TYPED-PTRS-DAG: %[[OUT_ST:[^ ]+]] = insertvalue %struct.outer %[[OUT_ST_ARR]], <2 x [8 x i32]*> %[[VEC]], 4
; CHECK-OPAQUE-PTRS-DAG: %[[OUT_ST_ARR:[^ ]+]] = insertvalue %struct.outer %[[OUT_ST_INN_ST]], [2 x ptr] %[[ARR]], 3
; CHECK-OPAQUE-PTRS-DAG: %[[OUT_ST:[^ ]+]] = insertvalue %struct.outer %[[OUT_ST_ARR]], <2 x ptr> %[[VEC]], 4

; CHECK-TYPED-PTRS: store %struct.outer %[[OUT_ST]], %struct.outer* %ptr.vec
; CHECK-OPAQUE-PTRS: store %struct.outer %[[OUT_ST]], ptr %ptr.vec

  ret void
}
