;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeLPG -vc-builtins-bif-path=%VC_BIF_XeLPG_TYPED_PTRS% \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-asmToConsole' -o /dev/null | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeLPG -vc-builtins-bif-path=%VC_BIF_XeLPG_OPAQUE_PTRS% \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-asmToConsole' -o /dev/null | FileCheck %s
; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P -vc-builtins-bif-path=%VC_BIF_Xe3P_TYPED_PTRS% \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-asmToConsole' -o /dev/null | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P -vc-builtins-bif-path=%VC_BIF_Xe3P_OPAQUE_PTRS% \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-asmToConsole' -o /dev/null | FileCheck %s

; CHECK-NOT: ERROR
target triple = "genx64-unknown-unknown"

; Function Attrs: mustprogress noinline nounwind willreturn
define dllexport spir_kernel void @foo_double_RTZ(i8 addrspace(1)* %in1buf, i8 addrspace(1)* nocapture readnone %in2buf, i8 addrspace(1)* %obuf, i64 %impl.arg.private.base) local_unnamed_addr #0 {
entry:
  %0 = ptrtoint i8 addrspace(1)* %in1buf to i64
  %1 = ptrtoint i8 addrspace(1)* %obuf to i64
  %2 = tail call i32 @llvm.genx.group.id.x() #5
  %mul.i = shl i32 %2, 7
  %conv2.i = zext i32 %mul.i to i64
  %add.i = add i64 %conv2.i, %0
  %3 = inttoptr i64 %add.i to <16 x double> addrspace(1)*
  %4 = load <16 x double>, <16 x double> addrspace(1)* %3, align 16
  %call.i10.i = tail call <16 x double> @llvm.genx.rsqrt.v16f64.v16f64(<16 x double> %4)
  %add5.i = add i64 %conv2.i, %1
  %5 = inttoptr i64 %add5.i to <16 x double> addrspace(1)*
  store <16 x double> %call.i10.i, <16 x double> addrspace(1)* %5, align 8
  ret void
}

; Function Attrs: mustprogress noinline nounwind willreturn
define dllexport spir_kernel void @foo_double_RTN(i8 addrspace(1)* %in1buf, i8 addrspace(1)* nocapture readnone %in2buf, i8 addrspace(1)* %obuf, i64 %impl.arg.private.base) local_unnamed_addr #1 {
entry:
  %0 = ptrtoint i8 addrspace(1)* %in1buf to i64
  %1 = ptrtoint i8 addrspace(1)* %obuf to i64
  %2 = tail call i32 @llvm.genx.group.id.x() #5
  %mul.i = shl i32 %2, 7
  %conv2.i = zext i32 %mul.i to i64
  %add.i = add i64 %conv2.i, %0
  %3 = inttoptr i64 %add.i to <16 x double> addrspace(1)*
  %4 = load <16 x double>, <16 x double> addrspace(1)* %3, align 16
  %call.i10.i = tail call <16 x double> @llvm.genx.rsqrt.v16f64.v16f64(<16 x double> %4)
  %add5.i = add i64 %conv2.i, %1
  %5 = inttoptr i64 %add5.i to <16 x double> addrspace(1)*
  store <16 x double> %call.i10.i, <16 x double> addrspace(1)* %5, align 8
  ret void
}

; Function Attrs: mustprogress noinline nounwind willreturn
define dllexport spir_kernel void @foo_double_RTP(i8 addrspace(1)* %in1buf, i8 addrspace(1)* nocapture readnone %in2buf, i8 addrspace(1)* %obuf, i64 %impl.arg.private.base) local_unnamed_addr #2 {
entry:
  %0 = ptrtoint i8 addrspace(1)* %in1buf to i64
  %1 = ptrtoint i8 addrspace(1)* %obuf to i64
  %2 = tail call i32 @llvm.genx.group.id.x() #5
  %mul.i = shl i32 %2, 7
  %conv2.i = zext i32 %mul.i to i64
  %add.i = add i64 %conv2.i, %0
  %3 = inttoptr i64 %add.i to <16 x double> addrspace(1)*
  %4 = load <16 x double>, <16 x double> addrspace(1)* %3, align 16
  %call.i10.i = tail call <16 x double> @llvm.genx.rsqrt.v16f64.v16f64(<16 x double> %4)
  %add5.i = add i64 %conv2.i, %1
  %5 = inttoptr i64 %add5.i to <16 x double> addrspace(1)*
  store <16 x double> %call.i10.i, <16 x double> addrspace(1)* %5, align 8
  ret void
}

; Function Attrs: mustprogress noinline nounwind willreturn
define dllexport spir_kernel void @foo_double_RTE(i8 addrspace(1)* %in1buf, i8 addrspace(1)* nocapture readnone %in2buf, i8 addrspace(1)* %obuf, i64 %impl.arg.private.base) local_unnamed_addr #3 {
entry:
  %0 = ptrtoint i8 addrspace(1)* %in1buf to i64
  %1 = ptrtoint i8 addrspace(1)* %obuf to i64
  %2 = tail call i32 @llvm.genx.group.id.x() #5
  %mul.i = shl i32 %2, 7
  %conv2.i = zext i32 %mul.i to i64
  %add.i = add i64 %conv2.i, %0
  %3 = inttoptr i64 %add.i to <16 x double> addrspace(1)*
  %4 = load <16 x double>, <16 x double> addrspace(1)* %3, align 16
  %call.i10.i = tail call <16 x double> @llvm.genx.rsqrt.v16f64.v16f64(<16 x double> %4)
  %add5.i = add i64 %conv2.i, %1
  %5 = inttoptr i64 %add5.i to <16 x double> addrspace(1)*
  store <16 x double> %call.i10.i, <16 x double> addrspace(1)* %5, align 8
  ret void
}

; Function Attrs: nofree nosync nounwind readnone
declare !genx_intrinsic_id !24 <16 x double> @llvm.genx.rsqrt.v16f64.v16f64(<16 x double>) #4

; Function Attrs: nofree nosync nounwind readnone
declare !genx_intrinsic_id !25 i32 @llvm.genx.group.id.x() #4

attributes #0 = { mustprogress noinline nounwind willreturn "CMFloatControl"="112" "CMGenxMain" "oclrt"="1" }
attributes #1 = { mustprogress noinline nounwind willreturn "CMFloatControl"="96" "CMGenxMain" "oclrt"="1" }
attributes #2 = { mustprogress noinline nounwind willreturn "CMFloatControl"="80" "CMGenxMain" "oclrt"="1" }
attributes #3 = { mustprogress noinline nounwind willreturn "CMFloatControl"="64" "CMGenxMain" "oclrt"="1" }
attributes #4 = { nofree nosync nounwind readnone }
attributes #5 = { nounwind }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.ocl.version = !{!1, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!5}
!spirv.Generator = !{!6}
!genx.kernels = !{!7, !12, !13, !14}
!llvm.ident = !{!15, !15, !15, !15, !15, !15, !15, !15}
!llvm.module.flags = !{!16}
!genx.kernel.internal = !{!17, !21, !22, !23}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{!"cl_doubles"}
!6 = !{i16 6, i16 14}
!7 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64)* @foo_double_RTZ, !"foo_double_RTZ", !8, i32 0, !9, !10, !11, i32 0}
!8 = !{i32 0, i32 0, i32 0, i32 96}
!9 = !{i32 72, i32 80, i32 88, i32 64}
!10 = !{i32 0, i32 0, i32 0}
!11 = !{!"svmptr_t", !"svmptr_t", !"svmptr_t"}
!12 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64)* @foo_double_RTN, !"foo_double_RTN", !8, i32 0, !9, !10, !11, i32 0}
!13 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64)* @foo_double_RTP, !"foo_double_RTP", !8, i32 0, !9, !10, !11, i32 0}
!14 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64)* @foo_double_RTE, !"foo_double_RTE", !8, i32 0, !9, !10, !11, i32 0}
!15 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
!16 = !{i32 1, !"wchar_size", i32 4}
!17 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64)* @foo_double_RTZ, !18, !19, !4, !20}
!18 = !{i32 0, i32 0, i32 0, i32 0}
!19 = !{i32 0, i32 1, i32 2, i32 3}
!20 = !{i32 255, i32 255, i32 255, i32 255}
!21 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64)* @foo_double_RTN, !18, !19, !4, !20}
!22 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64)* @foo_double_RTP, !18, !19, !4, !20}
!23 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i64)* @foo_double_RTE, !18, !19, !4, !20}
!24 = !{i32 10994}
!25 = !{i32 10884}
