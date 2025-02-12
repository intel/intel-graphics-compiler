;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --igc-merge-allocas -S %s --platformpvc | FileCheck %s
; ------------------------------------------------
; MergeAllocas
; ------------------------------------------------

; Check that allocas are not merged if optnone attribute is set
; Function Attrs: noinline optnone
define spir_kernel void @_ZTS43Kernel_NoReusePrivMem_SameFunc_AlwaysInline() #0 {
; CHECK-LABEL: _ZTS43Kernel_NoReusePrivMem_SameFunc_AlwaysInline
; CHECK-NEXT: alloca [128 x float], i32 0, align 4
; CHECK-NEXT: alloca [128 x float], i32 0, align 4
  %1 = alloca [128 x float], i32 0, align 4
  %2 = alloca [128 x float], i32 0, align 4
  ret void
}

attributes #0 = { noinline optnone }
