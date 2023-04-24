;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck --enable-var-scope %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@global = internal global i32 42, align 4

define internal void @func() #1 {
  ret void
}

define dllexport void @simple(void ()** %func.ptr.ptr, i32** %global.ptr.ptr) {
; CHECK-LABEL: @simple
; COM: all the lowered function pointers are at the function entry
; CHECK-DAG: %[[GADDR_FUNC:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0f_isVoidf(void ()* @func)
; CHECK-DAG: %[[PTR_FUNC:[^ ]+]] = inttoptr i64 %[[GADDR_FUNC]] to void ()*
; CHECK-DAG: %[[GADDR_GLOBAL:[^ ]+]] = call i64 @llvm.genx.gaddr.i64.p0i32(i32* @global)
; CHECK-DAG: %[[PTR_GLOBAL:[^ ]+]] = inttoptr i64 %[[GADDR_GLOBAL]] to i32*

  store void ()* @func, void ()** %func.ptr.ptr, align 8
; CHECK: store void ()* %[[PTR_FUNC]], void ()** %func.ptr.ptr, align 8
  store i32* @global, i32** %global.ptr.ptr, align 8
; CHECK: store i32* %[[PTR_GLOBAL]], i32** %global.ptr.ptr, align 8
  ret void
}

attributes #1 = { noinline nounwind "CMStackCall" }
