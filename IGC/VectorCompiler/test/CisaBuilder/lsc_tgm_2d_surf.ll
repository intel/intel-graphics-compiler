;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s



target triple = "genx64-unknown-unknown"

declare void @llvm.vc.internal.lsc.store.2d.tgm.surf.v2i8.v32i16(<2 x i8>, i64, i8, i32, i32, i32, i32, <32 x i16>)
declare <32 x i16> @llvm.vc.internal.lsc.load.2d.tgm.surf.v32i16.v2i8(<2 x i8>, i64, i8, i32, i32, i32, i32)

; define dllexport spir_kernel void @lsc_test(i8 addrspace(1)* nocapture readnone %arg, %intel.image2d_media_block_rw_t addrspace(1)* nocapture readnone %arg1, i32 %arg2, i32 %arg3, i64 %impl.arg.private.base) local_unnamed_addr #0 {
define spir_kernel void @test_typed(i64 %surf, i32 %x, i32 %y) local_unnamed_addr #0 {
; CHECK: lsc_load_block2d.tgm.uc.uc.uc  [[VX:V[0-9]+]]:16x2  surf([[SURF:V[0-9]+]])[[[X:V[0-9]+]],[[Y:V[0-9]+]]]
  %load = call <32 x i16> @llvm.vc.internal.lsc.load.2d.tgm.surf.v32i16.v2i8(<2 x i8> <i8 1, i8 1>, i64 %surf, i8 0, i32 2, i32 8, i32 %x, i32 %y)
; CHECK: lsc_store_block2d.tgm.uc.uc.uc  surf([[SURF]])[[[X]],[[Y]]]  [[VX]]:16x2
  call void @llvm.vc.internal.lsc.store.2d.tgm.surf.v2i8.v32i16(<2 x i8> <i8 1, i8 1>, i64 %surf, i8 0, i32 2, i32 8, i32 %x, i32 %y, <32 x i16> %load)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.ocl.version = !{!1, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}
!genx.kernels = !{!6}
!genx.kernel.internal = !{!11}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{i16 6, i16 14}
!6 = !{void (i64, i32, i32)* @test_typed, !"test_typed", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 0, i32 0, i32 0}
!8 = !{i32 64, i32 72, i32 76}
!9 = !{i32 0, i32 0, i32 0}
!10 = !{!"image2d_media_block_t"}
!11 = !{void (i64, i32, i32)* @test_typed, null, null, null, null}
