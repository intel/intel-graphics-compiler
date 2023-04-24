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

; CHECK:      [0] K2 (K)
; CHECK-NEXT:     v.0 S3
; CHECK-NEXT:     v.1 S2
; CHECK-NEXT: [1] K1 (K)
; CHECK-NEXT:     v.0 S3
; CHECK-NEXT:     v.1 S2
; CHECK-NEXT:     v.2 S1

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readnone
declare <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind
declare void @llvm.genx.media.st.v8i32(i32, i32, i32, i32, i32, i32, <8 x i32>) #1

; Function Attrs: noinline nounwind readnone
define internal spir_func { i32, <8 x i32> } @S3(<8 x i32> %0) unnamed_addr #2 !FuncArgSize !13 !FuncRetSize !14 {
  %.regioncollapsed1 = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %0, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %2 = add nsw i32 %.regioncollapsed1, 1
  %3 = insertelement <1 x i32> undef, i32 %2, i32 0
  %4 = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %0, <1 x i32> %3, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  %.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %4, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %5 = insertvalue { i32, <8 x i32> } undef, i32 %.regioncollapsed, 0
  %6 = insertvalue { i32, <8 x i32> } %5, <8 x i32> %4, 1
  ret { i32, <8 x i32> } %6
}

; Function Attrs: noinline nounwind readnone
define internal spir_func { i32, <8 x i32> } @S2(<8 x i32> %0) unnamed_addr #2 !FuncArgSize !13 !FuncRetSize !14 {
  %2 = tail call spir_func { i32, <8 x i32> } @S3(<8 x i32> %0) #4, !FuncArgSize !13, !FuncRetSize !14
  %ret = extractvalue { i32, <8 x i32> } %2, 0
  %3 = extractvalue { i32, <8 x i32> } %2, 1
  %.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %3, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %4 = add nsw i32 %.regioncollapsed, %ret
  %5 = insertvalue { i32, <8 x i32> } undef, i32 %4, 0
  %6 = insertvalue { i32, <8 x i32> } %5, <8 x i32> %3, 1
  ret { i32, <8 x i32> } %6
}

; Function Attrs: noinline nounwind readnone
define internal spir_func { i32, <8 x i32> } @S1(<8 x i32> %0) unnamed_addr #2 !FuncArgSize !13 !FuncRetSize !14 {
  %.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %0, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %2 = tail call spir_func { i32, <8 x i32> } @S2(<8 x i32> %0) #4, !FuncArgSize !13, !FuncRetSize !14
  %ret = extractvalue { i32, <8 x i32> } %2, 0
  %3 = extractvalue { i32, <8 x i32> } %2, 1
  %4 = add nsw i32 %ret, %.regioncollapsed
  %5 = insertvalue { i32, <8 x i32> } undef, i32 %4, 0
  %6 = insertvalue { i32, <8 x i32> } %5, <8 x i32> %3, 1
  ret { i32, <8 x i32> } %6
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @K1(i32 %0) local_unnamed_addr #3 {
  %2 = tail call spir_func { i32, <8 x i32> } @S1(<8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>) #4, !FuncArgSize !13, !FuncRetSize !14
  %ret = extractvalue { i32, <8 x i32> } %2, 0
  %3 = extractvalue { i32, <8 x i32> } %2, 1
  %.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %3, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %4 = add nsw i32 %.regioncollapsed, %ret
  %5 = insertelement <1 x i32> undef, i32 %4, i32 0
  %6 = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %3, <1 x i32> %5, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  tail call void @llvm.genx.media.st.v8i32(i32 0, i32 %0, i32 0, i32 32, i32 0, i32 0, <8 x i32> %6)
  ret void
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @K2(i32 %0) local_unnamed_addr #3 {
  %2 = tail call spir_func { i32, <8 x i32> } @S2(<8 x i32> <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>) #4, !FuncArgSize !13, !FuncRetSize !14
  %ret = extractvalue { i32, <8 x i32> } %2, 0
  %3 = extractvalue { i32, <8 x i32> } %2, 1
  %.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %3, i32 0, i32 1, i32 1, i16 4, i32 undef)
  %4 = add nsw i32 %.regioncollapsed, %ret
  %5 = insertelement <1 x i32> undef, i32 %4, i32 0
  %6 = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %3, <1 x i32> %5, i32 0, i32 1, i32 0, i16 4, i32 undef, i1 true)
  tail call void @llvm.genx.media.st.v8i32(i32 0, i32 %0, i32 0, i32 32, i32 0, i32 0, <8 x i32> %6)
  ret void
}

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !12 i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32) #0

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind readnone "CMStackCall" }
attributes #3 = { noinline nounwind "CMGenxMain" }
attributes #4 = { noinline nounwind }

!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4, !9}
!VC.Debug.Enable = !{}
!genx.kernel.internal = !{!10, !11}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i32)* @K1, !"K1", !5, i32 0, !6, !7, !8, i32 0}
!5 = !{i32 2}
!6 = !{i32 64}
!7 = !{i32 0}
!8 = !{!"buffer_t read_write"}
!9 = !{void (i32)* @K2, !"K2", !5, i32 0, !6, !7, !8, i32 0}
!10 = !{void (i32)* @K1, !7, !7, null, null}
!11 = !{void (i32)* @K2, !7, !7, null, null}
!12 = !{i32 7724}
!13 = !{i32 1}
!14 = !{i32 1}
