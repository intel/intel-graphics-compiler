;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that kernel arg offsets pass will not touch arguments with IO annotations.

; RUN: %opt_typed_ptrs %use_old_pass_manager% -CMKernelArgOffset -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -CMKernelArgOffset -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMKernelArgOffset -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMKernelArgOffset -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define dllexport spir_kernel void @barfoo(i32 %0, <6 x i32> %1) #0 {
  ret void
}

attributes #0 = { "CMGenxMain" }

; CHECK: !genx.kernels = !{[[KERNEL:![0-9]+]]}
!genx.kernels = !{!0}
!genx.kernel.internal = !{!3}

; CHECK-TYPED-PTRS: [[KERNEL]] = !{void (i32, <6 x i32>)* @barfoo, !"barfoo", !1, i32 0, [[OFFSETS:![0-9]+]]
; CHECK-OPAQUE-PTRS: [[KERNEL]] = !{ptr @barfoo, !"barfoo", !1, i32 0, [[OFFSETS:![0-9]+]]
; CHECK: [[OFFSETS]] = !{i32 64, i32 68}
!0 = !{void (i32, <6 x i32>)* @barfoo, !"barfoo", !1, i32 0, i32 0, !4, !2, i32 0}
!1 = !{i32 0, i32 0}
!2 = !{!"", !""}
!3 = !{void (i32, <6 x i32>)* @barfoo, null, null, null, null}
!4 = !{i32 4, i32 4}
