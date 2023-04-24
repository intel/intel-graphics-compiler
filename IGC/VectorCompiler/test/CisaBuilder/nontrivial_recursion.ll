;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: the test just checks that this IR can be compiled successfully
; RUN: llc %s -march=genx64 -mcpu=Gen9 -mattr=+ocl_runtime -o /dev/null

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readnone
declare <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind
declare void @llvm.genx.media.st.v8i32(i32, i32, i32, i32, i32, i32, <8 x i32>) #1

; Function Attrs: noinline nounwind readnone
define internal spir_func { i32, <8 x i32> } @S1(<8 x i32> %0) unnamed_addr #2 {
  %sev.cast.13.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %0, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %2 = add nsw i32 %sev.cast.13.regioncollapsed, 2
  %sev.cast.2 = insertelement <1 x i32> undef, i32 %2, i64 0
  %3 = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %0, <1 x i32> %sev.cast.2, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  %4 = tail call spir_func <8 x i32> @S2(<8 x i32> %3) #4
  %sev.cast.4.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %4, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %5 = insertvalue { i32, <8 x i32> } undef, i32 %sev.cast.4.regioncollapsed, 0
  %6 = insertvalue { i32, <8 x i32> } %5, <8 x i32> %4, 1
  ret { i32, <8 x i32> } %6
}

; Function Attrs: noinline nounwind readnone
define internal spir_func <8 x i32> @S2(<8 x i32> %0) unnamed_addr #2 {
  %sev.cast.13.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %0, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %2 = add nsw i32 %sev.cast.13.regioncollapsed, 3
  %sev.cast.2 = insertelement <1 x i32> undef, i32 %2, i64 0
  %3 = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %0, <1 x i32> %sev.cast.2, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  %4 = tail call spir_func { i32, <8 x i32> } @S1(<8 x i32> %3) #4
  %5 = extractvalue { i32, <8 x i32> } %4, 1
  ret <8 x i32> %5
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @K1(i32 %0, i64 %privBase) local_unnamed_addr #3 {
  %2 = tail call spir_func { i32, <8 x i32> } @S1(<8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>) #4
  %ret = extractvalue { i32, <8 x i32> } %2, 0
  %3 = extractvalue { i32, <8 x i32> } %2, 1
  %sev.cast.22.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %3, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %4 = add nsw i32 %sev.cast.22.regioncollapsed, %ret
  %sev.cast.1 = insertelement <1 x i32> undef, i32 %4, i64 0
  %5 = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> %3, <1 x i32> %sev.cast.1, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  tail call void @llvm.genx.media.st.v8i32(i32 0, i32 0, i32 0, i32 32, i32 0, i32 0, <8 x i32> %5)
  ret void
}

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !11 i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32) #0

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind readnone "CMStackCall" }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" }
attributes #4 = { noinline nounwind }

!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!9}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i32, i64)* @K1, !"K1", !5, i32 0, !6, !7, !8, i32 0}
!5 = !{i32 2, i32 96}
!6 = !{i32 72, i32 64}
!7 = !{i32 0}
!8 = !{!"buffer_t read_write"}
!9 = !{void (i32, i64)* @K1, !0, !10, !2, !10}
!10 = !{i32 0, i32 1}
!11 = !{i32 7747}
