;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s

; COM: Function pointer stores write the byte offset of the element.

target datalayout = "e-p:64:64-i64:64-n8:16:32"

define internal spir_func void @foo() addrspace(9) {
  ret void
}

define internal spir_func void @bar() addrspace(9) {
  ret void
}

define dllexport void @f_f(i64 %idx) {
  %buf = alloca [4 x void () addrspace(9)*], align 8
  %p0 = getelementptr inbounds [4 x void () addrspace(9)*], [4 x void () addrspace(9)*]* %buf, i64 0, i64 0
  store void () addrspace(9)* @foo, void () addrspace(9)** %p0, align 8
; CHECK: call <4 x i64> @llvm.genx.wrregioni.v4i64.i64.i16.i1(<4 x i64> %{{[^ ]+}}, i64 ptrtoint ({{.*}} @foo to i64), i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
  %p2 = getelementptr inbounds [4 x void () addrspace(9)*], [4 x void () addrspace(9)*]* %buf, i64 0, i64 2
  store void () addrspace(9)* @bar, void () addrspace(9)** %p2, align 8
; CHECK: call <4 x i64> @llvm.genx.wrregioni.v4i64.i64.i16.i1(<4 x i64> %{{[^ ]+}}, i64 ptrtoint ({{.*}} @bar to i64), i32 0, i32 1, i32 1, i16 16, i32 0, i1 true)
  %pi = getelementptr inbounds [4 x void () addrspace(9)*], [4 x void () addrspace(9)*]* %buf, i64 0, i64 %idx
  store void () addrspace(9)* @foo, void () addrspace(9)** %pi, align 8
; CHECK: %[[OFF:[^ ]+]] = mul i{{[0-9]+}} %{{[^ ]+}}, 8
; CHECK: %[[OFF16:[^ ]+]] = trunc i{{[0-9]+}} %[[OFF]] to i16
; CHECK: call <4 x i64> @llvm.genx.wrregioni.v4i64.i64.i16.i1(<4 x i64> %{{[^ ]+}}, i64 ptrtoint ({{.*}} @foo to i64), i32 0, i32 1, i32 1, i16 %[[OFF16]], i32 0, i1 true)
  ret void
}
