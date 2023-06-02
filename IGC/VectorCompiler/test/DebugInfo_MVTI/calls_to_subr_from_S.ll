;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dump-module-to-visa-transform-info-path=%basename_t.structure \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: FileCheck %s --input-file=%basename_t.structure
; CHECK:      [0] main_kernel (K)
; CHECK-NEXT:     l.0 _Z24__cm_intrinsic_impl_test
; CHECK-NEXT:     v.0 _Z6callee15cm_surfaceindexu2CMvr8_x
; CHECK-NEXT:         l.0 _Z24__cm_intrinsic_impl_test

; ModuleID = 'calls_to_subr_from_S.ll'
source_filename = "calls_to_subr_from_S.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: noinline nounwind readnone
define spir_func <8 x i64> @_Z24__cm_intrinsic_impl_test(<8 x i64> %0, <8 x i64> %1) local_unnamed_addr #0 {
  %3 = add <8 x i64> %0, %1
  ret <8 x i64> %3
}

; Function Attrs: noinline nounwind
define internal spir_func void @_Z6callee15cm_surfaceindexu2CMvr8_x(<8 x i64> %0) unnamed_addr #1 !FuncArgSize !10 !FuncRetSize !11 {
  %res = tail call spir_func <8 x i64> @_Z24__cm_intrinsic_impl_test(<8 x i64> %0, <8 x i64> %0)
  tail call void @llvm.genx.oword.st.v8i64(i32 3, i32 0, <8 x i64> %res)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>) #2

; Function Attrs: noinline nounwind
define dllexport void @main_kernel(i32 %0, i32 %1, i32 %2, i64 %privBase) local_unnamed_addr #3 {
  %4 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 1, i32 0)
  %5 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 2, i32 0)
  %6 = add <8 x i64> %5, %4
  %7 = tail call spir_func <8 x i64> @_Z24__cm_intrinsic_impl_test(<8 x i64> %6, <8 x i64> %6)
  tail call spir_func void @_Z6callee15cm_surfaceindexu2CMvr8_x(<8 x i64> %7), !FuncArgSize !10, !FuncRetSize !11
  ret void
}

; Function Attrs: nounwind readonly
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32) #4

attributes #0 = { noinline nounwind readnone "target-cpu"="Gen9" "VC.Emulation.Routine" }
attributes #1 = { noinline nounwind "CMStackCall" "target-cpu"="Gen9" }
attributes #2 = { nounwind "target-cpu"="Gen9" }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" "target-cpu"="Gen9" }
attributes #4 = { nounwind readonly "target-cpu"="Gen9" }

!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!VC.Debug.Enable = !{}
!genx.kernel.internal = !{!9}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i32, i32, i32, i64)* @main_kernel, !"main_kernel", !5, i32 0, !6, !7, !8, i32 0}
!5 = !{i32 2, i32 2, i32 2, i32 96}
!6 = !{i32 136, i32 144, i32 152, i32 128}
!7 = !{i32 0, i32 0, i32 0}
!8 = !{!"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write"}
!9 = !{void (i32, i32, i32, i64)* @main_kernel, null, null, null, null}
!10 = !{i32 2}
!11 = !{i32 0}
