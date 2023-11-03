;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %opt %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLP \
; RUN: -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s | FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; Gen9 VISA check
; CHECK: movs (M1_NM, 1) %bss({{[0-9]}}) V{{[0-9].*}}(

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

@llvm.vc.predef.var.bss = external global i32 #0

define dllexport spir_kernel void @simple(i32 %surf, i32 %samp) #1 {
  call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %surf)
  ret void
}

declare !genx_intrinsic_id !6 void @llvm.genx.write.predef.surface.p0i32(i32*, i32) #2

attributes #0 = { "VCPredefinedVariable" }
attributes #1 = { "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="Gen9" }
attributes #2 = { nounwind writeonly }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32, i32)* @simple, !"simple", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 1}
!2 = !{i32 32, i32 96}
!3 = !{i32 0, i32 0}
!4 = !{!"buffer_t read_write", !"sampler_t"}
!5 = !{void (i32, i32)* @simple, null, null, null, null}
!6 = !{i32 11175}