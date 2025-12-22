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

@global = internal global i32 42, align 4

define internal void @func() #1 {
  ret void
}

define dllexport void @simple(void ()** %func.ptr.ptr, i32** %global.ptr.ptr) {
; CHECK-LABEL: @simple
; COM: all the lowered function pointers are at the function entry
; CHECK-TYPED-PTRS-DAG: %[[GADDR_FUNC:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func)
; CHECK-TYPED-PTRS-DAG: %[[PTR_FUNC:[^ ]+]] = inttoptr i64 %[[GADDR_FUNC]] to void ()*
; CHECK-TYPED-PTRS-DAG: %[[GADDR_GLOBAL:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0i32(i32* @global)
; CHECK-TYPED-PTRS-DAG: %[[PTR_GLOBAL:[^ ]+]] = inttoptr i64 %[[GADDR_GLOBAL]] to i32*
; CHECK-OPAQUE-PTRS-DAG: %[[GADDR_FUNC:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0(ptr @func)
; CHECK-OPAQUE-PTRS-DAG: %[[PTR_FUNC:[^ ]+]] = inttoptr i64 %[[GADDR_FUNC]] to ptr
; CHECK-OPAQUE-PTRS-DAG: %[[GADDR_GLOBAL:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0(ptr @global)
; CHECK-OPAQUE-PTRS-DAG: %[[PTR_GLOBAL:[^ ]+]] = inttoptr i64 %[[GADDR_GLOBAL]] to ptr

  store void ()* @func, void ()** %func.ptr.ptr, align 8
; CHECK-TYPED-PTRS: store void ()* %[[PTR_FUNC]], void ()** %func.ptr.ptr, align 8
; CHECK-OPAQUE-PTRS: store ptr %[[PTR_FUNC]], ptr %func.ptr.ptr, align 8
  store i32* @global, i32** %global.ptr.ptr, align 8
; CHECK-TYPED-PTRS: store i32* %[[PTR_GLOBAL]], i32** %global.ptr.ptr, align 8
; CHECK-OPAQUE-PTRS: store ptr %[[PTR_GLOBAL]], ptr %global.ptr.ptr, align 8
  ret void
}

attributes #1 = { noinline nounwind "CMStackCall" }
