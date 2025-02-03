;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMABI -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMABI -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@global.int = internal global i32 0

; COM: the test checks that if no align was specified, the respected alloca
; COM: should not have one too. Starting from LLVM 11, this semantic is
; COM: represented through 'align 1'.
; FIXME: Make 'align 1' an unconditional part of the check after LLVM 9-10
;        gets abolished.
; CHECK: %global.int.local = alloca i32{{(, align 1)?}}
; CHECK-TYPED-PTRS: store i32 0, i32* %global.int.local{{(, align 1)?}}
; CHECK-OPAQUE-PTRS: store i32 0, ptr %global.int.local{{(, align 1)?}}
define dllexport void @kernel(float %kernel.value) {
  %1 = load i32, i32* @global.int, align 4
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (float)* @kernel}
