;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dump-module-to-visa-transform-info-path=%basename_t.structure \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: FileCheck %s --input-file=%basename_t.structure
; CHECK:      [0] test_kernel (K)
; CHECK-NEXT:     l.0 _Z24__cm_intrinsic_impl_test

; ModuleID = 'calls_to_subr_from_K.ll'
source_filename = "calls_to_subr_from_K.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind readonly
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32) #0

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>) #1

; Function Attrs: noinline nounwind readnone
define spir_func <8 x i64> @_Z24__cm_intrinsic_impl_test(<8 x i64> %0, <8 x i64> %1) local_unnamed_addr #2 {
  %3 = add <8 x i64> %0, %1
  ret <8 x i64> %3
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test_kernel(i32 %0, i32 %1) local_unnamed_addr #3 {
  %3 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %4 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %5 = tail call spir_func <8 x i64> @_Z24__cm_intrinsic_impl_test(<8 x i64> %3, <8 x i64> %4) #4
  tail call void @llvm.genx.oword.st.v8i64(i32 %0, i32 2, <8 x i64> %5)
  ret void
}

attributes #0 = { nounwind readonly "target-cpu"="Gen9" }
attributes #1 = { nounwind "target-cpu"="Gen9" }
attributes #2 = { noinline nounwind readnone "target-cpu"="Gen9" "VC.Emulation.Routine" }
attributes #3 = { noinline nounwind "CMGenxMain" "target-cpu"="Gen9" }
attributes #4 = { nounwind readnone }

!opencl.enable.FP_CONTRACT = !{}
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
!4 = !{void (i32, i32)* @test_kernel, !"test_kernel", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @test_kernel, null, null, null, null}
