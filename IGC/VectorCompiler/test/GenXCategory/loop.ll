;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

%intel.image2d_media_block_rw_t = type opaque

; Function Attrs: nounwind
declare void @llvm.genx.media.st.v16i32(i32, i32, i32, i32, i32, i32, <16 x i32>) #0

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test(%intel.image2d_media_block_rw_t addrspace(1)* nocapture readnone %arg, i32 %arg3, i32 %arg4, i64 %impl.arg.private.base) local_unnamed_addr #1 {
  %icmp = icmp sgt i32 %arg3, 0
  br i1 %icmp, label %.preheader.lr.ph, label %.._crit_edge36_crit_edge

.._crit_edge36_crit_edge:                         ; preds = %0
  br label %._crit_edge36

; CHECK-LABEL: .preheader.lr.ph:
; CHECK:      [[CONST1:[^ ]+]] = call i32 @llvm.genx.constanti.i32(i32 0)
; CHECK-NEXT: [[CONST3:[^ ]+]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> zeroinitializer)
; CHECK-NEXT: [[CONST4:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32> [[CONST3]], i32 0, i32 16, i32 0, i16 0, i32 undef)
; CHECK-NEXT: br label %.preheader
.preheader.lr.ph:                                 ; preds = %0
  %icmp5 = icmp sgt i32 %arg4, 0
  br label %.preheader

; CHECK-LABEL: .preheader:
; CHECK: phi i32 [ [[CONST1]], %.preheader.lr.ph ], [ %add6, %._crit_edge..preheader_crit_edge ]
.preheader:                                       ; preds = %._crit_edge..preheader_crit_edge, %.preheader.lr.ph
  %.03035 = phi i32 [ 0, %.preheader.lr.ph ], [ %add6, %._crit_edge..preheader_crit_edge ]
  br i1 %icmp5, label %.preheader..lr.ph_crit_edge, label %.preheader.._crit_edge_crit_edge

.preheader.._crit_edge_crit_edge:                 ; preds = %.preheader
  br label %._crit_edge

.preheader..lr.ph_crit_edge:                      ; preds = %.preheader
  br label %.lr.ph

; CHECK-LABEL: .lr.ph:
; CHECK: [[CONST2:[^ ]+]] = call i32 @llvm.genx.convert.i32(i32 1)
; CHECK-NEXT: tail call void @llvm.genx.media.st.v16i32(i32 0, i32 [[CONST2]], i32 0, i32 64, i32 %.03035, i32 %.033, <16 x i32> [[CONST4]])
.lr.ph:                                           ; preds = %.lr.ph..lr.ph_crit_edge, %.preheader..lr.ph_crit_edge
  %.033 = phi i32 [ %add, %.lr.ph..lr.ph_crit_edge ], [ 0, %.preheader..lr.ph_crit_edge ]
  tail call void @llvm.genx.media.st.v16i32(i32 0, i32 1, i32 0, i32 64, i32 %.03035, i32 %.033, <16 x i32> zeroinitializer)
  %add = add nuw nsw i32 %.033, 1, !spirv.Decorations !17
  %exitcond.not = icmp eq i32 %add, %arg4
  br i1 %exitcond.not, label %.lr.ph.._crit_edge_crit_edge, label %.lr.ph..lr.ph_crit_edge

.lr.ph..lr.ph_crit_edge:                          ; preds = %.lr.ph
  br label %.lr.ph

.lr.ph.._crit_edge_crit_edge:                     ; preds = %.lr.ph
  br label %._crit_edge

._crit_edge:                                      ; preds = %.lr.ph.._crit_edge_crit_edge, %.preheader.._crit_edge_crit_edge
  %add6 = add nuw nsw i32 %.03035, 1, !spirv.Decorations !17
  %exitcond37.not = icmp eq i32 %add6, %arg3
  br i1 %exitcond37.not, label %._crit_edge.._crit_edge36_crit_edge, label %._crit_edge..preheader_crit_edge

._crit_edge..preheader_crit_edge:                 ; preds = %._crit_edge
  br label %.preheader

._crit_edge.._crit_edge36_crit_edge:              ; preds = %._crit_edge
  br label %._crit_edge36

._crit_edge36:                                    ; preds = %._crit_edge.._crit_edge36_crit_edge, %.._crit_edge36_crit_edge
  ret void
}

attributes #0 = { nounwind }
attributes #1 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }

!spirv.MemoryModel = !{!0}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!1}
!opencl.spir.version = !{!2, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.ocl.version = !{!1, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3, !3}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!5}
!genx.kernels = !{!6}
!genx.kernel.internal = !{!11}
!llvm.ident = !{!15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15, !15}
!llvm.module.flags = !{!16}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{i16 6, i16 14}
!6 = !{void (%intel.image2d_media_block_rw_t addrspace(1)*, i32, i32, i64)* @test, !"test", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 2, i32 0, i32 0, i32 96}
!8 = !{i32 -1, i32 136, i32 140, i32 128}
!9 = !{i32 0, i32 0, i32 0}
!10 = !{!"image2d_media_block_t read_write", !"", !""}
!11 = !{void (%intel.image2d_media_block_rw_t addrspace(1)*, i32, i32, i64)* @test, !12, !13, !4, !14}
!12 = !{i32 0, i32 0, i32 0, i32 0}
!13 = !{i32 0, i32 1, i32 2, i32 3}
!14 = !{i32 1, i32 -1, i32 -1, i32 255}
!15 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
!16 = !{i32 1, !"wchar_size", i32 4}
!17 = !{!18}
!18 = !{i32 4469}
