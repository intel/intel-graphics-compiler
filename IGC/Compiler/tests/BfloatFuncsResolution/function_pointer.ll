;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-14-plus
;
; RUN: igc_opt %s -S -o - --igc-bfloat-funcs-resolution | FileCheck %s

; This test verifies if BfloatFuncsResolution pass will not crash when function pointer is provided.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_select
define spir_kernel void @test_select(i1 %cond) #1 {
entry:

  %ptr_func_a = ptrtoint void ()* @func_a to i64
  %ptr_func_b = ptrtoint void ()* @func_b to i64
  %func_int = select i1 %cond, i64 %ptr_func_a, i64 %ptr_func_b
  %func_ptr = inttoptr i64 %func_int to void ()*
  call void %func_ptr()
; CHECK: ret void
  ret void
}


declare void @func_a()
declare void @func_b()


attributes #0 = { nounwind "referenced-indirectly" "visaStackCall" }
attributes #1 = { convergent nounwind }
attributes #2 = { nounwind }