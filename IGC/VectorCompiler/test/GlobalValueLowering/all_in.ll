;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

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
; CHECK-DAG: %array_a.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_a)
; CHECK-DAG: %array_a.lowered = inttoptr i64 %array_a.gaddr to [8 x i32]*
; CHECK-DAG: %array_b.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_b)
; CHECK-DAG: %array_b.lowered = inttoptr i64 %array_b.gaddr to [8 x i32]*
; CHECK-DAG: %array_c.gaddr = call i64 @llvm.genx.gaddr.i64.p0a8i32([8 x i32]* @array_c)
; CHECK-DAG: %array_c.lowered = inttoptr i64 %array_c.gaddr to [8 x i32]*

; COM: lower GEP
; CHECK-DAG: %[[GEP:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %array_a.lowered, i32 0, i64 3

; COM: lower inner struct
; CHECK-DAG: %[[INN_ST_HALF:[^ ]+]] = insertvalue %struct.inner undef, [8 x i32]* %array_a.lowered, 0
; CHECK-DAG: %[[INN_ST_GEP:[^ ]+]] = getelementptr inbounds [8 x i32], [8 x i32]* %array_a.lowered, i32 0, i64 5
; CHECK-DAG: %[[INN_ST:[^ ]+]] = insertvalue %struct.inner %[[INN_ST_HALF]], i32* %[[INN_ST_GEP]], 1

; COM: lower array
; CHECK-DAG: %[[ARR_HALF:[^ ]+]] = insertvalue [2 x [8 x i32]*] undef, [8 x i32]* %array_b.lowered, 0
; CHECK-DAG: %[[ARR:[^ ]+]] = insertvalue [2 x [8 x i32]*] %[[ARR_HALF]], [8 x i32]* %array_c.lowered, 1

; COM: lower vector
; CHECK-DAG: %[[VEC_HALF:[^ ]+]] = insertelement <2 x [8 x i32]*> undef, [8 x i32]* %array_c.lowered, i64 0
; CHECK-DAG: %[[VEC:[^ ]+]] = insertelement <2 x [8 x i32]*> %[[VEC_HALF]], [8 x i32]* %array_a.lowered, i64 1

; COM: lower outer struct
; CHECK-DAG: %[[OUT_ST_GEP:[^ ]+]] = insertvalue %struct.outer { i32 23, i32* undef, %struct.inner undef, [2 x [8 x i32]*] undef, <2 x [8 x i32]*> undef }, i32* %[[GEP]], 1
; CHECK-DAG: %[[OUT_ST_INN_ST:[^ ]+]] = insertvalue %struct.outer %[[OUT_ST_GEP]], %struct.inner %[[INN_ST]], 2
; CHECK-DAG: %[[OUT_ST_ARR:[^ ]+]] = insertvalue %struct.outer %[[OUT_ST_INN_ST]], [2 x [8 x i32]*] %[[ARR]], 3
; CHECK-DAG: %[[OUT_ST:[^ ]+]] = insertvalue %struct.outer %[[OUT_ST_ARR]], <2 x [8 x i32]*> %[[VEC]], 4

; CHECK: store %struct.outer %[[OUT_ST]], %struct.outer* %ptr.vec

  ret void
}
