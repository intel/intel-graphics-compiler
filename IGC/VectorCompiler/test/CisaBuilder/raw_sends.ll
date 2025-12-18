;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P -mattr=-efficient_64b_enabled -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P -mattr=-efficient_64b_enabled -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s

; RUN: %not_for_vc_diag% llc %s -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null 2>&1 | \
; RUN: FileCheck --check-prefix=CHECK-SENDG %s

declare <16 x i32> @llvm.genx.r0.v16i32()
declare void @llvm.genx.raw.send2.noresult.i1.v16i32(i8, i8, i1, i8, i8, i32, i32, <16 x i32>)

; CHECK: mov (M1, 16) [[PAYLOAD:V[0-9]+]](0,0)<1> %r0(0,0)<1;1,0>
; CHECK: raw_sends_eot.3.1.0.0 (M1, 1)  0x0:ud 0x2000010:ud [[PAYLOAD]].0 %null.0 %null.0

; CHECK-SENDG: error: LLVM ERROR: GenXCisaBuilder failed for:
; CHECK-SENDG-SAME: call void @llvm.genx.raw.send2.noresult.i1.v16i32(i8 2, i8 0, i1 true, i8 1, i8 3, i32 0, i32 33554448, <16 x i32> %r0)
; CHECK-SENDG-SAME: Intrinsic is not supported in efficient 64-bit addressing mode

define dllexport spir_kernel void @test(i64 %buffer) local_unnamed_addr #0 {
  %r0 = call <16 x i32> @llvm.genx.r0.v16i32()
  call void @llvm.genx.raw.send2.noresult.i1.v16i32(i8 2, i8 0, i1 true, i8 1, i8 3, i32 0, i32 33554448, <16 x i32> %r0)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!0}
!opencl.used.optional.core.features = !{!0}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{}
!1 = !{i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i16 6, i16 14}
!4 = !{void (i64)* @test, !"test", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0}
!6 = !{i32 64}
!7 = !{!"svmptr_t"}
!8 = !{void (i64)* @test, null, null, null, null}
