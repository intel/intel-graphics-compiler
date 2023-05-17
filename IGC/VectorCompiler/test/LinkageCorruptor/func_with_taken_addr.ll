;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLinkageCorruptor -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@simple_global_int = internal global i32 42, align 4
@simple_const_float = internal constant float 4.200000e+01, align 4
@__imparg_llvm.vc.internal.print.buffer = internal global i64 undef

; COM: Fake functions to obtain implicit args inside extern/indirect functions.
; COM: In real case scenario there'll be some code in place of those functions,
; COM: e.g r0 -> and -> i2p -> gep -> load. But the code itself doesn't affect
; COM: global variable lowering flow.
declare <3 x i16> @get_loc_id()
declare <3 x i32> @get_loc_sz()
declare <3 x i32> @get_grp_cnt()
declare i64 @get_printf_ptr()

; Function Attrs: noinline nounwind
define internal spir_func float @bar() {
  %simple_global_int.load = load i32, i32* @simple_global_int, align 4
  %simple_global_int.inc = add nsw i32 %simple_global_int.load, 1
  store i32 %simple_global_int.inc, i32* @simple_global_int, align 4
  %simple_const_float.load = load float, float* @simple_const_float, align 4
  %simple_const_float.inc = fadd float %simple_const_float.load, 1.000000e+00
  ret float %simple_const_float.inc
}

; COM: function with taken address shouldn't change
define internal spir_func void @indirect() {
; CHECK: define internal spir_func void @indirect() #[[ATTR:[0-9]]] {
  %indirect.get.ptr = call i64 @get_printf_ptr()
  store i64 %indirect.get.ptr, i64* @__imparg_llvm.vc.internal.print.buffer, align 8
  %indirect.int.load = load i64, i64* @__imparg_llvm.vc.internal.print.buffer, align 8
  ret void
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @foo_kernel() {
  %ret.val = call spir_func float @bar()
  %just.use = fadd float %ret.val, 1.000000e+00
  %indirect.user = ptrtoint void ()* @indirect to i32
  ret void
}

; CHECK: attributes #[[ATTR]] = { "CMStackCall" }

!genx.kernels = !{!0}
!0 = !{void ()* @foo_kernel}
