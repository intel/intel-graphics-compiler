;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXSLMResolution -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXSLMResolution -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

declare void @llvm.genx.slm.init(i32)

define dllexport spir_kernel void @kernel() {
    ; CHECK-NOT: call void @llvm.genx.slm.init(i32 12345)
    call void @llvm.genx.slm.init(i32 12345)
    ret void
}

!genx.kernels = !{!0}
!genx.kernel.internal = !{!2}

; CHECK-TYPED-PTRS: !0 = !{void ()* @kernel, !"kernel", !1, i32 12345, !1, !1, !1, i32 0}
; CHECK-OPAQUE-PTRS: !0 = !{ptr @kernel, !"kernel", !1, i32 12345, !1, !1, !1, i32 0}
!0 = !{void ()* @kernel, !"kernel", !1, i32 1234, !1, !1, !1, i32 0}
!1 = !{}
!2 = !{void ()* @kernel, !1, !1, !1, !1}
