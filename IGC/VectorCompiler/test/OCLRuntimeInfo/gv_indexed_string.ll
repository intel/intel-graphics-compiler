;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 -mattr=+ocl_runtime -vc-analyze=GenXOCLRuntimeInfo \
; RUN: -vc-choose-pass-manager-override=false -o /dev/null 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK: Printing analysis 'GenXOCLRuntimeInfo':

@str = internal addrspace(2) constant [5 x i8] c"text\00", align 1

declare i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)*)

; COM: Indexed string must not be encoded in any section.
; CHECK: ModuleInfo:
; CHECK: Constant
; CHECK: Data:
; CHECK-NOT: Buffer:
; CHECK-NOT: Symbols:

; CHECK: Global:
; CHECK: Data:
; CHECK-NOT: Buffer:
; CHECK-NOT: Symbols:

define dllexport spir_kernel void @kernel() #0 {
  %gep = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @str, i64 0, i64 0
  %idx = tail call i32 @llvm.vc.internal.print.format.index.p2i8(i8 addrspace(2)* %gep)
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!1}
!genx.kernel.internal = !{!2}

!0 = !{}
!1 = !{void ()* @kernel, !"kernel", !0, i32 0, !0, !0, !0, i32 0}
!2 = !{void ()* @kernel, !0, !0, null, !0}
