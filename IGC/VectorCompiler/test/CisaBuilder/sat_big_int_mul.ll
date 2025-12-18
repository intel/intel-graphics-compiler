;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s

declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v8i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <8 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <16 x i32>)

declare <16 x i32> @llvm.genx.uumul.sat.v16i32.v16i32(<16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.ssmul.sat.v16i32.v16i32(<16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.sumul.sat.v16i32.v16i32(<16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.usmul.sat.v16i32.v16i32(<16 x i32>, <16 x i32>)

declare <16 x i16> @llvm.genx.uumul.sat.v16i16.v16i16(<16 x i16>, <16 x i16>)
declare <16 x i16> @llvm.genx.ssmul.sat.v16i16.v16i16(<16 x i16>, <16 x i16>)
declare <16 x i16> @llvm.genx.sumul.sat.v16i16.v16i16(<16 x i16>, <16 x i16>)
declare <16 x i16> @llvm.genx.usmul.sat.v16i16.v16i16(<16 x i16>, <16 x i16>)

; CHECK-LABEL: mul32_kernel
; CHECK-NOT: mul.sat
define dllexport spir_kernel void @mul32_kernel(<16 x i32> %a, <16 x i32> %b, <16 x i32> %c, <16 x i32> %d, i64 %block) local_unnamed_addr #0 {
  %uu.mul = tail call <16 x i32> @llvm.genx.uumul.sat.v16i32.v16i32(<16 x i32> %a, <16 x i32> %b)
  %ss.mul = tail call <16 x i32> @llvm.genx.ssmul.sat.v16i32.v16i32(<16 x i32> %c, <16 x i32> %d)
  %su.mul = tail call <16 x i32> @llvm.genx.sumul.sat.v16i32.v16i32(<16 x i32> %uu.mul, <16 x i32> %b)
  %us.mul = tail call <16 x i32> @llvm.genx.usmul.sat.v16i32.v16i32(<16 x i32> %ss.mul, <16 x i32> %c)
  %res = add <16 x i32> %su.mul, %us.mul
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <16 x i32> %res)
  ret void
}

; CHECK-LABEL: mul16_kernel
define dllexport spir_kernel void @mul16_kernel(<16 x i16> %a, <16 x i16> %b, <16 x i16> %c, <16 x i16> %d, i64 %block) local_unnamed_addr #0 {
; CHECK: mul.sat
  %uu.mul = tail call <16 x i16> @llvm.genx.uumul.sat.v16i16.v16i16(<16 x i16> %a, <16 x i16> %b)
; CHECK: mul.sat
  %ss.mul = tail call <16 x i16> @llvm.genx.ssmul.sat.v16i16.v16i16(<16 x i16> %c, <16 x i16> %d)
; CHECK: mul.sat
  %su.mul = tail call <16 x i16> @llvm.genx.sumul.sat.v16i16.v16i16(<16 x i16> %uu.mul, <16 x i16> %b)
; CHECK: mul.sat
  %us.mul = tail call <16 x i16> @llvm.genx.usmul.sat.v16i16.v16i16(<16 x i16> %ss.mul, <16 x i16> %c)
  %res = add <16 x i16> %su.mul, %us.mul
  %cast = bitcast <16 x i16> %res to <8 x i32>
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v8i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <8 x i32> %cast)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!0}
!opencl.used.optional.core.features = !{!0}
!spirv.Generator = !{!3}
!genx.kernels = !{!4, !9}
!genx.kernel.internal = !{!8, !13}

!0 = !{}
!1 = !{i32 0}
!2 = !{i32 1, i32 1}
!3 = !{i16 6, i16 14}
!4 = !{void (<16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, i64)* @mul32_kernel, !"mul32_kernel", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0, i32 0, i32 0, i32 0, i32 0}
!6 = !{i32 64, i32 128, i32 192, i32 256, i32 320}
!7 = !{!""}
!8 = !{void (<16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, i64)* @mul32_kernel, null, null, null, null}

!9 = !{void (<16 x i16>, <16 x i16>, <16 x i16>, <16 x i16>, i64)* @mul16_kernel, !"mul16_kernel", !10, i32 0, !6, !11, !12, i32 0}
!10 = !{i32 0, i32 0, i32 0, i32 0, i32 0}
!11 = !{i32 32, i32 64, i32 96, i32 128, i32 160}
!12 = !{!""}
!13 = !{void (<16 x i16>, <16 x i16>, <16 x i16>, <16 x i16>, i64)* @mul16_kernel, null, null, null, null}
