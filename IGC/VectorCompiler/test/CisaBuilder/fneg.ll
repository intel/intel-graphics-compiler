;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPassWrapper -GenXFinalizer \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -finalizer-opts="-dumpcommonisa -isaasmToConsole" -mcpu=Gen9 < %s | FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; Gen9 VISA check
; CHECK: oword_ld (2) T{{[0-9]}} 0x0:ud [[TMP0:[A-z0-9]*]].0
; CHECK: mov (M1, 8) [[TMP1:[A-z0-9]*]](0,0)<1> (-)[[TMP0]](0,0)<1;1,0>
; CHECK: oword_st (2) T{{[0-9]}} 0x0:ud [[TMP1]].0

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

declare <8 x float> @llvm.genx.oword.ld.v8f32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v8f32(i32, i32, <8 x float>) #0

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @the_test(i32 %0, i32 %1) local_unnamed_addr #1 {
  %vec = tail call <8 x float> @llvm.genx.oword.ld.v8f32(i32 0, i32 %0, i32 0) #2
  %res = fneg <8 x float> %vec
  tail call void @llvm.genx.oword.st.v8f32(i32 %1, i32 0, <8 x float> %res) #2
  ret void
}

attributes #0 = { "target-cpu"="Gen9" }
attributes #1 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="Gen9" }
attributes #2 = { nounwind }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i32, i32)* @the_test, !"the_test", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @the_test, null, null, null, null}
