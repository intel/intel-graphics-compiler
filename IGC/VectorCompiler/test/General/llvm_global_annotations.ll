;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 -o /dev/null
; ModuleID = 'Deserialized LLVM Module'
target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readonly
declare <64 x i32> @llvm.genx.media.ld.v64i32(i32, i32, i32, i32, i32, i32) #0

; Function Attrs: nounwind
declare void @llvm.genx.media.st.v64i32(i32, i32, i32, i32, i32, i32, <64 x i32>) #1

@annotation = private unnamed_addr constant [11 x i8] c"annotation\00", section "llvm.metadata"
@llvm.global.annotations = appending global [1 x { i8*, i8*, i8*, i32 }] [{ i8*, i8*, i8*, i32 } { i8* bitcast (void (i32, i32, i32, <3 x i16>, <3 x i32>, i64)* @annotated_kernel to i8*), i8* getelementptr inbounds ([11 x i8], [11 x i8]* @annotation, i32 0, i32 0), i8* undef, i32 undef }], section "llvm.metadata"

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @annotated_kernel(i32 %0, i32 %1, i32 %2, <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i32> %impl.arg.llvm.genx.local.size, i64 %impl.arg.private.base) local_unnamed_addr #2 {
  %call.i.i.i = tail call i32 @llvm.genx.group.id.x() #1
  %call.i2.i.i89 = tail call i32 @llvm.genx.group.id.y() #1
  %vecext.i.i85 = extractelement <3 x i32> %impl.arg.llvm.genx.local.size, i32 0
  %vecext.i.i82 = extractelement <3 x i32> %impl.arg.llvm.genx.local.size, i32 1
  %4 = mul i32 %call.i.i.i, %vecext.i.i85
  %call.i.i.i75 = zext <3 x i16> %impl.arg.llvm.genx.local.id16 to <3 x i32>
  %vecext.i.i76 = extractelement <3 x i32> %call.i.i.i75, i32 0
  %vecext.i.i73 = extractelement <3 x i32> %call.i.i.i75, i32 1
  %5 = add i32 %4, %vecext.i.i76
  %6 = shl i32 %5, 3
  %7 = mul i32 %call.i2.i.i89, %vecext.i.i82
  %8 = add i32 %7, %vecext.i.i73
  %9 = shl i32 %8, 3
  %10 = tail call <64 x i32> @llvm.genx.media.ld.v64i32(i32 0, i32 1, i32 0, i32 32, i32 %6, i32 %9)
  %11 = insertelement <64 x i32> undef, i32 %2, i32 0
  %12 = shufflevector <64 x i32> %11, <64 x i32> undef, <64 x i32> zeroinitializer
  %13 = add <64 x i32> %10, %12
  tail call void @llvm.genx.media.st.v64i32(i32 0, i32 2, i32 0, i32 32, i32 %6, i32 %9, <64 x i32> %13)
  ret void
}

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !16 i32 @llvm.genx.group.id.x() #3

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !17 i32 @llvm.genx.group.id.y() #3

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind "CMGenxMain" }
attributes #3 = { nounwind readnone }

!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!0}
!opencl.spir.version = !{!1, !2, !2, !2}
!opencl.ocl.version = !{!0, !2, !2, !2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!llvm.ident = !{!10, !10, !10}
!llvm.module.flags = !{!11}
!genx.kernel.internal = !{!12}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{i32 2, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (i32, i32, i32, <3 x i16>, <3 x i32>, i64)* @annotated_kernel, !"annotated_kernel", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 2, i32 2, i32 0, i32 24, i32 8, i32 96}
!7 = !{i32 -1, i32 -1, i32 88, i32 32, i32 64, i32 80}
!8 = !{i32 0, i32 0, i32 0}
!9 = !{!"image2d_media_block_t read_write", !"image2d_media_block_t read_write", !""}
!10 = !{!"clang version 10.0.0 (c850858c57d75a2670ceceea8f7bb1ba8e3803ce)"}
!11 = !{i32 1, !"wchar_size", i32 4}
!12 = !{void (i32, i32, i32, <3 x i16>, <3 x i32>, i64)* @annotated_kernel, !13, !14, !3, !15}
!13 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!14 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5}
!15 = !{i32 1, i32 2, i32 -1, i32 255, i32 255, i32 255}
!16 = !{i32 7666}
!17 = !{i32 7667}
