;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Xe2 -S < %s | FileCheck --enable-var-scope %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Xe2 -S < %s | FileCheck --enable-var-scope %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

define internal void @func_a() #1 {
  ret void
}

define internal void @func_b() #1 {
  ret void
}

define dllexport void @simple(i1 %bool) {
; CHECK-LABEL: @simple
; COM: all the lowered function pointers are at the function entry
; CHECK-TYPED-PTRS-DAG: %[[GADDR_A:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func_a)
; CHECK-TYPED-PTRS-DAG: %[[PTR_A:[^ ]+]] = inttoptr i64 %[[GADDR_A]] to void ()*
; CHECK-TYPED-PTRS-DAG: %[[GADDR_B:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func_b)
; CHECK-TYPED-PTRS-DAG: %[[PTR_B:[^ ]+]] = inttoptr i64 %[[GADDR_B]] to void ()*
; CHECK-OPAQUE-PTRS-DAG: %[[GADDR_A:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0(ptr @func_a)
; CHECK-OPAQUE-PTRS-DAG: %[[PTR_A:[^ ]+]] = inttoptr i64 %[[GADDR_A]] to ptr
; CHECK-OPAQUE-PTRS-DAG: %[[GADDR_B:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0(ptr @func_b)
; CHECK-OPAQUE-PTRS-DAG: %[[PTR_B:[^ ]+]] = inttoptr i64 %[[GADDR_B]] to ptr

  %func.ptr = select i1 %bool, void ()* @func_a, void ()* @func_b
; CHECK-TYPED-PTRS: %func.ptr = select i1 %bool, void ()* %[[PTR_A]], void ()* %[[PTR_B]]
; CHECK-OPAQUE-PTRS: %func.ptr = select i1 %bool, ptr %[[PTR_A]], ptr %[[PTR_B]]
  call void %func.ptr()
; CHECK: call void %func.ptr()
  ret void
}

define dllexport void @ispc_icmp(<4 x i64> %func.ptrs) {
; CHECK-LABEL: @ispc_icmp
; CHECK-TYPED-PTRS-DAG: %[[GADDR_A:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func_a)
; CHECK-TYPED-PTRS-DAG: %[[PTR_A:[^ ]+]] = inttoptr i64 %[[GADDR_A]] to void ()*
; CHECK-TYPED-PTRS-DAG: %[[GADDR_B:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func_b)
; CHECK-TYPED-PTRS-DAG: %[[PTR_B:[^ ]+]] = inttoptr i64 %[[GADDR_B]] to void ()*
; CHECK-TYPED-PTRS-DAG: %[[P2I_A:[^ ]+]] = ptrtoint void ()* %[[PTR_A]] to i64
; CHECK-TYPED-PTRS-DAG: %[[P2I_B:[^ ]+]] = ptrtoint void ()* %[[PTR_B]] to i64
; CHECK-OPAQUE-PTRS-DAG: %[[GADDR_A:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0(ptr @func_a)
; CHECK-OPAQUE-PTRS-DAG: %[[PTR_A:[^ ]+]] = inttoptr i64 %[[GADDR_A]] to ptr
; CHECK-OPAQUE-PTRS-DAG: %[[GADDR_B:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0(ptr @func_b)
; CHECK-OPAQUE-PTRS-DAG: %[[PTR_B:[^ ]+]] = inttoptr i64 %[[GADDR_B]] to ptr
; CHECK-OPAQUE-PTRS-DAG: %[[P2I_A:[^ ]+]] = ptrtoint ptr %[[PTR_A]] to i64
; CHECK-OPAQUE-PTRS-DAG: %[[P2I_B:[^ ]+]] = ptrtoint ptr %[[PTR_B]] to i64
; CHECK-DAG: %[[VEC_0:[^ ]+]] = insertelement <4 x i64> undef, i64 %[[P2I_A]], i64 0
; CHECK-DAG: %[[VEC_1:[^ ]+]] = insertelement <4 x i64> %[[VEC_0]], i64 %[[P2I_B]], i64 1
; CHECK-DAG: %[[VEC_2:[^ ]+]] = insertelement <4 x i64> %[[VEC_1]], i64 %[[P2I_A]], i64 2
; CHECK-DAG: %[[VEC:[^ ]+]] = insertelement <4 x i64> %[[VEC_2]], i64 %[[P2I_B]], i64 3

; COM: <a, b, a, b>
  %icmp = icmp eq <4 x i64> %func.ptrs, <i64 ptrtoint (void ()* @func_a to i64), i64 ptrtoint (void ()* @func_b to i64), i64 ptrtoint (void ()* @func_a to i64), i64 ptrtoint (void ()* @func_b to i64)>
; CHECK: %icmp = icmp eq <4 x i64> %func.ptrs, %[[VEC]]
  ret void
}

attributes #1 = { noinline nounwind "CMStackCall" }
