;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck --enable-var-scope %s

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
; CHECK-DAG: %[[GADDR_A:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func_a)
; CHECK-DAG: %[[PTR_A:[^ ]+]] = inttoptr i64 %[[GADDR_A]] to void ()*
; CHECK-DAG: %[[GADDR_B:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func_b)
; CHECK-DAG: %[[PTR_B:[^ ]+]] = inttoptr i64 %[[GADDR_B]] to void ()*

  %func.ptr = select i1 %bool, void ()* @func_a, void ()* @func_b
; CHECK: %func.ptr = select i1 %bool, void ()* %[[PTR_A]], void ()* %[[PTR_B]]
  call void %func.ptr()
; CHECK: call void %func.ptr()
  ret void
}

define dllexport void @ispc_icmp(<4 x i64> %func.ptrs) {
; CHECK-LABEL: @ispc_icmp
; CHECK-DAG: %[[GADDR_A:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func_a)
; CHECK-DAG: %[[PTR_A:[^ ]+]] = inttoptr i64 %[[GADDR_A]] to void ()*
; CHECK-DAG: %[[GADDR_B:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func_b)
; CHECK-DAG: %[[PTR_B:[^ ]+]] = inttoptr i64 %[[GADDR_B]] to void ()*
; CHECK-DAG: %[[P2I_A:[^ ]+]] = ptrtoint void ()* %[[PTR_A]] to i64
; CHECK-DAG: %[[P2I_B:[^ ]+]] = ptrtoint void ()* %[[PTR_B]] to i64
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
