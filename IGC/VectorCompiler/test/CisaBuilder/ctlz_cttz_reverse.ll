;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=XeHPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s

target triple = "genx64-unknown-unknown"

; CHECK: bfrev (M1, 16)
; CHECK: bfrev (M1, 16)
; CHECK: lzd (M1, 16)
; CHECK: lzd (M1, 16)
; CHECK: lzd (M1, 16)
; CHECK: lzd (M1, 16)

declare <32 x i32> @llvm.genx.oword.ld.v32i32(i32, i32, i32)
declare void @llvm.genx.oword.st.v32i32(i32, i32, <32 x i32>)

declare <32 x i32> @llvm.cttz.v32i32(<32 x i32>, i1)
declare <32 x i32> @llvm.ctlz.v32i32(<32 x i32>, i1)

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @the_test(i32 %0, i32 %1) local_unnamed_addr #0 {
  %vec = tail call <32 x i32> @llvm.genx.oword.ld.v32i32(i32 0, i32 %0, i32 0)
  %res.cttz = call <32 x i32>  @llvm.cttz.v32i32(<32 x i32> %vec, i1 false)
  %res.ctlz = call <32 x i32>  @llvm.ctlz.v32i32(<32 x i32> %vec, i1 false)
  tail call void @llvm.genx.oword.st.v32i32(i32 %1, i32 0, <32 x i32> %res.cttz)
  tail call void @llvm.genx.oword.st.v32i32(i32 %0, i32 0, <32 x i32> %res.ctlz)
  ret void
}

attributes #0 = { mustprogress nofree noinline nosync nounwind willreturn "CMGenxMain" "oclrt"="1" }

!spirv.MemoryModel = !{!9}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}
!llvm.ident = !{!10, !10, !10}
!llvm.module.flags = !{!11}


!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i32, i32)* @the_test, !"the_test", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @the_test, null, null, null, null}
!9 = !{i32 2, i32 2}
!10 = !{!"Ubuntu clang version 14.0.6"}
!11 = !{i32 1, !"wchar_size", i32 4}
