;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGlobalUniformAnalysis -print-global-uniform-info \
; RUN: -march=genx64 -mcpu=XeHPG -disable-output -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGlobalUniformAnalysis -print-global-uniform-info \
; RUN: -march=genx64 -mcpu=XeHPG -disable-output -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK: Non-uniform basic blocks:
; CHECK-DAG: BB1
; CHECK-DAG: BB2
; CHECK-DAG: BB3
; CHECK-DAG: BB4
; CHECK-EMPTY:

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readonly
declare <8 x i32> @llvm.genx.media.ld.v8i32(i32, i32, i32, i32, i32, i32) #0

; Function Attrs: nounwind readonly
declare <1 x i32> @llvm.genx.media.ld.v1i32(i32, i32, i32, i32, i32, i32) #0

; Function Attrs: nounwind
declare void @llvm.genx.media.st.v8i32(i32, i32, i32, i32, i32, i32, <8 x i32>) #1

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test(i32 %arg, i32 %arg4, i32 %arg5, i32 %arg6, <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i32> %impl.arg.llvm.genx.local.size, i64 %impl.arg.private.base) local_unnamed_addr #2 {
BB0:
  %call.i.i.i = tail call i32 @llvm.genx.group.id.x() #1
  %vecext.i.i932 = call i32 @llvm.genx.rdregioni.i32.v3i32.i16(<3 x i32> %impl.arg.llvm.genx.local.size, i32 0, i32 1, i32 1, i16 0, i32 0)
  %mul = mul i32 %call.i.i.i, %vecext.i.i932
  %rdregioni = call i16 @llvm.genx.rdregioni.i16.v3i16.i16(<3 x i16> %impl.arg.llvm.genx.local.id16, i32 0, i32 1, i32 1, i16 0, i32 0)
  %vecext.i.i84 = zext i16 %rdregioni to i32
  %add = add i32 %mul, %vecext.i.i84
  %shl = shl i32 %add, 3
  %.categoryconv16 = call i32 @llvm.genx.convert.i32(i32 1)
  %ld = tail call <8 x i32> @llvm.genx.media.ld.v8i32(i32 0, i32 %.categoryconv16, i32 0, i32 32, i32 %shl, i32 0)
  %.categoryconv14 = call i32 @llvm.genx.convert.i32(i32 3)
  %ld7 = tail call <1 x i32> @llvm.genx.media.ld.v1i32(i32 0, i32 %.categoryconv14, i32 0, i32 4, i32 %add, i32 0)
  %sev.cast.113 = bitcast <1 x i32> %ld7 to i32
  %icmp = icmp eq i32 %sev.cast.113, 0
  br i1 %icmp, label %BB4, label %BB1

BB1:                                              ; preds = %BB0
  %icmp8 = icmp eq i32 %arg6, 0
  br i1 %icmp8, label %BB5, label %BB2

BB2:                                              ; preds = BB4
  %constant = call i32 @llvm.genx.constanti.i32(i32 0)
  br label %BB3

BB3:                                              ; preds = %BB3, %BB2
  %phi = phi <8 x i32> [ %add9, %BB3 ], [ %ld, %BB2 ]
  %.097 = phi i32 [ %add10, %BB3 ], [ %constant, %BB2 ]
  %add9 = add <8 x i32> %phi, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %add10 = add nuw i32 %.097, 1
  %exitcond = icmp eq i32 %add10, %arg6
  br i1 %exitcond, label %BB5, label %BB3

BB4:                                              ; preds = %BB0
  %shl11 = shl <8 x i32> %ld, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  br label %BB5

BB5:                                              ; preds = %BB3, %BB1, %BB4
  %.1 = phi <8 x i32> [ %shl11, %BB4 ], [ %ld, %BB1 ], [ %add9, %BB3 ]
  %.categoryconv = call i32 @llvm.genx.convert.i32(i32 2)
  tail call void @llvm.genx.media.st.v8i32(i32 0, i32 %.categoryconv, i32 0, i32 32, i32 %shl, i32 0, <8 x i32> %.1)
  ret void
}

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !16 i32 @llvm.genx.group.id.x() #3

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !17 i16 @llvm.genx.rdregioni.i16.v3i16.i16(<3 x i16>, i32, i32, i32, i16, i32) #3

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !17 i32 @llvm.genx.rdregioni.i32.v3i32.i16(<3 x i32>, i32, i32, i32, i16, i32) #3

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !18 i32 @llvm.genx.constanti.i32(i32) #3

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !19 i32 @llvm.genx.convert.i32(i32) #3

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" }
attributes #3 = { nounwind readnone }
attributes #4 = { nounwind writeonly }

!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!0}
!opencl.spir.version = !{!1, !2, !2, !2, !2, !2, !2, !2}
!opencl.ocl.version = !{!0, !2, !2, !2, !2, !2, !2, !2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!llvm.ident = !{!10, !10, !10, !10, !10, !10, !10}
!llvm.module.flags = !{!11}
!genx.kernel.internal = !{!12}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{i32 2, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (i32, i32, i32, i32, <3 x i16>, <3 x i32>, i64)* @test, !"test", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 2, i32 2, i32 2, i32 0, i32 24, i32 8, i32 96}
!7 = !{i32 -1, i32 -1, i32 -1, i32 152, i32 64, i32 128, i32 144}
!8 = !{i32 0, i32 0, i32 0, i32 0}
!9 = !{!"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !""}
!10 = !{!"clang version 10.0.0 (c850858c57d75a2670ceceea8fBB4ba8e3803ce)"}
!11 = !{i32 1, !"wchar_size", i32 4}
!12 = !{void (i32, i32, i32, i32, <3 x i16>, <3 x i32>, i64)* @test, !13, !14, !3, !15}
!13 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!14 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6}
!15 = !{i32 1, i32 2, i32 3, i32 -1, i32 255, i32 255, i32 255}
!16 = !{i32 7666}
!17 = !{i32 7753}
!18 = !{i32 7571}
!19 = !{i32 7573}
