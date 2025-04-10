;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: when coalescing is disabled current implementation is expected
; COM: to create extra copies of extractvalues...

; COM: test is expected just to run compilation without extra checks
; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Gen9  -mattr=+ocl_runtime \
; RUN: -vc-disable-coalescing \
; RUN: -o /dev/null
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Gen9  -mattr=+ocl_runtime \
; RUN: -vc-disable-coalescing \
; RUN: -o /dev/null


target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readonly
declare <16 x i32> @llvm.genx.oword.ld.v16i32(i32, i32, i32) #1

; Function Attrs: nounwind readnone
declare { <16 x i32>, <16 x i32> } @llvm.genx.subb.v16i32.v16i32(<16 x i32>, <16 x i32>) #0

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v16i32(i32, i32, <16 x i32>) #2

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @wobble(i32 %0, i32 %1, i32 %2, i32 %3, i64 %privBase) local_unnamed_addr #3 {
  %5 = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 0, i32 0)
  %6 = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 1, i32 0)
  %7 = tail call { <16 x i32>, <16 x i32> } @llvm.genx.subb.v16i32.v16i32(<16 x i32> %5, <16 x i32> %6)
  %8 = extractvalue { <16 x i32>, <16 x i32> } %7, 0
  %9 = extractvalue { <16 x i32>, <16 x i32> } %7, 1
  tail call void @llvm.genx.oword.st.v16i32(i32 3, i32 0, <16 x i32> %9)
  tail call void @llvm.genx.oword.st.v16i32(i32 2, i32 128, <16 x i32> %8)
  ret void
}

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

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
!4 = !{void (i32, i32, i32, i32, i64)* @"wobble", !"wobble", !5, i32 0, !6, !7, !8, i32 0}
!5 = !{i32 2, i32 2, i32 2, i32 2, i32 96}
!6 = !{i32 88, i32 96, i32 104, i32 112, i32 80}
!7 = !{i32 0, i32 0, i32 0, i32 0}
!8 = !{!"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write"}
!9 = !{void (i32, i32, i32, i32, i64)* @"wobble", !10, !11, !2, !11}
!10 = !{i32 0, i32 0, i32 0, i32 0, i32 0}
!11 = !{i32 0, i32 1, i32 2, i32 3, i32 4}
