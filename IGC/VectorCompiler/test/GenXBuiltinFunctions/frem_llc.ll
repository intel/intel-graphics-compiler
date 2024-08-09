;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=XeLP -vc-builtins-bif-path=%VC_BUILTINS_BIF_XeLP% \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-asmToConsole' -o /dev/null | FileCheck %s

; CHECK-NOT: ERROR
target triple = "genx64-unknown-unknown"

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn
define dllexport spir_kernel void @mod_f32_5_5_5_49052222([5 x float] addrspace(1)* nocapture readonly %0, [5 x float] addrspace(1)* nocapture readonly %1, [5 x float] addrspace(1)* nocapture writeonly %2, <3 x i16> %impl.arg.llvm.genx.local.id16, i64 %impl.arg.private.base) local_unnamed_addr #0 {
  %4 = extractelement <3 x i16> %impl.arg.llvm.genx.local.id16, i64 0
  %5 = zext i16 %4 to i64
  %6 = getelementptr [5 x float], [5 x float] addrspace(1)* %0, i64 0, i64 %5
  %7 = load float, float addrspace(1)* %6, align 4
  %8 = getelementptr [5 x float], [5 x float] addrspace(1)* %1, i64 0, i64 %5
  %9 = load float, float addrspace(1)* %8, align 4
  %10 = frem float %7, %9
  %11 = getelementptr [5 x float], [5 x float] addrspace(1)* %2, i64 0, i64 %5
  store float %10, float addrspace(1)* %11, align 4
  ret void
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn "CMGenxMain" }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}
!genx.kernels = !{!6}
!genx.kernel.internal = !{!11}

!0 = !{i32 2, i32 2}
!1 = !{i32 3, i32 102000}
!2 = !{i32 1, i32 2}
!3 = !{!"cl_khr_fp16"}
!4 = !{!"cl_doubles"}
!5 = !{i16 0, i16 22}
!6 = !{void ([5 x float] addrspace(1)*, [5 x float] addrspace(1)*, [5 x float] addrspace(1)*, <3 x i16>, i64)* @mod_f32_5_5_5_49052222, !"mod_f32_5_5_5_49052222", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 0, i32 0, i32 0, i32 24, i32 96}
!8 = !{i32 136, i32 144, i32 152, i32 64, i32 128}
!9 = !{i32 0, i32 0, i32 0}
!10 = !{!"svmptr_t", !"svmptr_t", !"svmptr_t"}
!11 = !{void ([5 x float] addrspace(1)*, [5 x float] addrspace(1)*, [5 x float] addrspace(1)*, <3 x i16>, i64)* @mod_f32_5_5_5_49052222, !12, !13, !14, !15}
!12 = !{i32 0, i32 0, i32 0, i32 0, i32 0}
!13 = !{i32 0, i32 1, i32 2, i32 3, i32 4}
!14 = !{}
!15 = !{i32 255, i32 255, i32 255, i32 255, i32 255}
