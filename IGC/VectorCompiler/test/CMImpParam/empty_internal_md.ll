;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=true -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define dllexport spir_kernel void @kernel() {
  ret void
}

!genx.kernels = !{!0}
; CHECK: !genx.kernel.internal = !{![[INT_MD_NODE:[0-9]+]]}

!0 = !{void ()* @kernel, !"kernel", !1, i32 0, !1, !1, !1, i32 0, i32 0}
!1 = !{}
; CHECK: ![[INT_MD_NODE]] = !{void ()* @kernel, null, null, null, null}
