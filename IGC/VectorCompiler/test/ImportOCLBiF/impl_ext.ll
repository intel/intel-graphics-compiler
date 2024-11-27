;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% %pass_pref%GenXImportOCLBiF -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

declare spir_func double @_Z3expd(double)

; COM: currently all *__cm_intrinsic_impl_* are deemed to have an external
; linkage (needed for emulation pass)
; COM: the test checks that external linkage specifier is preserved
; CHECK: define void @__cm_intrinsic_impl_foo
define external void @__cm_intrinsic_impl_foo() {
  ret void
}
