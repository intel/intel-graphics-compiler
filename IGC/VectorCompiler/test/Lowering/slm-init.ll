;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare void @llvm.genx.slm.init(i32)

define dllexport spir_kernel void @kernel() {
    ; CHECK-NOT: call void @llvm.genx.slm.init(i32 12345)
    call void @llvm.genx.slm.init(i32 12345)
    ret void
}

!genx.kernels = !{!0}
!genx.kernel.internal = !{!2}

; CHECK: !0 = !{void ()* @kernel, !"kernel", !1, i32 12345, !1, !1, !1, i32 0}
!0 = !{void ()* @kernel, !"kernel", !1, i32 1234, !1, !1, !1, i32 0}
!1 = !{}
!2 = !{void ()* @kernel, !1, !1, !1, !1}
