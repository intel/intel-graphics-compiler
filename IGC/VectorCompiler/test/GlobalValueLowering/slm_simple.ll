;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGlobalValueLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@slm_i1 = internal unnamed_addr addrspace(3) global i1 undef
@slm_v2i8 = internal unnamed_addr addrspace(3) global <2 x i8> undef
@slm_struct_i64i8 = internal unnamed_addr addrspace(3) global { i64, i8 } undef
@slm_struct_align = internal unnamed_addr addrspace(3) global { i1 } undef, align 256

define dllexport spir_kernel void @kernel() #0 {
  ; CHECK: %slm_struct_align.lowered = inttoptr i32 0 to { i1 } addrspace(3)*
  ; CHECK: %slm_struct_i64i8.lowered = inttoptr i32 8 to { i64, i8 } addrspace(3)*
  ; CHECK: %slm_v2i8.lowered = inttoptr i32 24 to <2 x i8> addrspace(3)*
  ; CHECK: %slm_i1.lowered = inttoptr i32 26 to i1 addrspace(3)*

  ; CHECK: %load_i1 = load i1, i1 addrspace(3)* %slm_i1.lowered
  %load_i1 = load i1, i1 addrspace(3)* @slm_i1

  ; CHECK: %load_v2i8 = load <2 x i8>, <2 x i8> addrspace(3)* %slm_v2i8.lowered
  %load_v2i8 = load <2 x i8>, <2 x i8> addrspace(3)* @slm_v2i8

  ; CHECK: %load_struct_i64i8 = load { i64, i8 }, { i64, i8 } addrspace(3)* %slm_struct_i64i8.lowered
  %load_struct_i64i8 = load { i64, i8 }, { i64, i8 } addrspace(3)* @slm_struct_i64i8

  ; CHECK: %load_struct_align = load { i1 }, { i1 } addrspace(3)* %slm_struct_align.lowered
  %load_struct_align = load { i1 }, { i1 } addrspace(3)* @slm_struct_align
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!3}

; CHECK: !{{[[:digit:]]}} = !{void ()* @kernel, !"kernel", !{{[[:digit:]]}}, i32 27, !{{[[:digit:]]}}, !{{[[:digit:]]}}, !{{[[:digit:]]}}, i32 0}
!0 = !{void ()* @kernel, !"kernel", !1, i32 0, !1, !1, !2, i32 0}
!1 = !{}
!2 = !{!""}
!3 = !{void ()* @kernel, !1, !1, !1, !1}
