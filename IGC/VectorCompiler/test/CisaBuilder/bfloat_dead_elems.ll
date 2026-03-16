;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; Test that bfloat fadd with dead elements is split to a narrow width
; and the finalizer does not widen it back (unlike half).

; RUN: %llc_typed_ptrs %s -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' \
; RUN: -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -o /dev/null | FileCheck %s
; RUN: %llc_opaque_ptrs %s -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' \
; RUN: -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -o /dev/null | FileCheck %s

; Check that the bfloat add uses a narrow exec size (not widened to 16).
; CHECK: add (M1, 2)
; CHECK-NOT: add (M1, 16)

define dllexport spir_kernel void @kernel_bf_dead(<16 x bfloat> %in1, <16 x bfloat> %in2, i8 addrspace(1)* nocapture writeonly %out, i64 %impl.arg.private.base) local_unnamed_addr #0 {
entry:
  %add = fadd <16 x bfloat> %in1, %in2
  %int = fptoui <16 x bfloat> %add to <16 x i16>
  %and = and <16 x i16> %int, <i16 1, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0>
  %bc = bitcast <16 x i16> %and to <16 x bfloat>
  %0 = bitcast i8 addrspace(1)* %out to <16 x bfloat> addrspace(1)*
  store <16 x bfloat> %bc, <16 x bfloat> addrspace(1)* %0, align 4
  ret void
}

attributes #0 = { mustprogress nofree noinline norecurse nosync nounwind willreturn writeonly "CMGenxMain" "oclrt"="1" }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!genx.kernel.internal = !{!10}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (<16 x bfloat>, <16 x bfloat>, i8 addrspace(1)*, i64)* @kernel_bf_dead, !"kernel_bf_dead", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 0, i32 0, i32 0, i32 96}
!7 = !{i32 96, i32 160, i32 224, i32 64}
!8 = !{i32 0, i32 0, i32 0}
!9 = !{!"", !"", !"svmptr_t"}
!10 = !{void (<16 x bfloat>, <16 x bfloat>, i8 addrspace(1)*, i64)* @kernel_bf_dead, !11, !12, !3, !13}
!11 = !{i32 0, i32 0, i32 0, i32 0}
!12 = !{i32 0, i32 1, i32 2, i32 3}
!13 = !{i32 -1, i32 -1, i32 255, i32 255}
