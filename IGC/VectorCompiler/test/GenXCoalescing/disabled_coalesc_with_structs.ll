;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: check that GenXCoalescing works in a specific case with struct arguments

; RUN: opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCoalescingWrapper \
; RUN:  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -vc-disable-coalescing -S \
; RUN:  < %s | FileCheck %s

; ModuleID = 'reduced.ll'
source_filename = "before_coalesc_0.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8>, i32, i32, i32, i16, i32) #0

define internal spir_func { <16 x i8>, <16 x i8>, <16 x i8> } @_Z9Calculatett15cm_surfaceindex15cm_surfaceindex15cm_surfaceindex15cm_surfaceindexfiu2CMmb2x8_fS_u2CMmr2x8_hS0_S0_() unnamed_addr #0 {
.preheader470:
  br label %0

0:                                                ; preds = %.preheader470
  br label %.preheader469.1.preheader

.preheader469.1.preheader:                        ; preds = %0
  br label %1

1:                                                ; preds = %.preheader469.1.preheader
  br label %.preheader468.1.preheader

.preheader468.1.preheader:                        ; preds = %1
  %rdregioni407 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> undef, i32 64, i32 16, i32 4, i16 0, i32 undef)
  br label %2

2:                                                ; preds = %.preheader468.1.preheader
  br label %.preheader.1.preheader

.preheader.1.preheader:                           ; preds = %2
  %rdregioni463 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> undef, i32 64, i32 16, i32 4, i16 0, i32 undef)
  %rdregioni506 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> undef, i32 64, i32 16, i32 4, i16 0, i32 undef)
  %insertvalue507 = insertvalue { <16 x i8>, <16 x i8>, <16 x i8> } undef, <16 x i8> %rdregioni407, 0
  %insertvalue508 = insertvalue { <16 x i8>, <16 x i8>, <16 x i8> } %insertvalue507, <16 x i8> %rdregioni463, 1
  %insertvalue = insertvalue { <16 x i8>, <16 x i8>, <16 x i8> } %insertvalue508, <16 x i8> %rdregioni506, 2
  ret { <16 x i8>, <16 x i8>, <16 x i8> } %insertvalue
}

declare dllexport spir_kernel void @analyze_motion_1(i32, i32, i32, i32, i32, float, i64) local_unnamed_addr #0

define dllexport spir_kernel void @analyze_motion_2(i32 %arg, i32 %arg47, i32 %arg48, i32 %arg49, i32 %arg50, i32 %arg51, i32 %arg52, i32 %arg53, i32 %arg54, i32 %arg55, i32 %arg56, i32 %arg57, float %arg58, i64 %privBase) local_unnamed_addr #1 {
._crit_edge:
  br label %1

0:                                                ; No predecessors!
  %call = call spir_func { <16 x i8>, <16 x i8>, <16 x i8> } @_Z9Calculatett15cm_surfaceindex15cm_surfaceindex15cm_surfaceindex15cm_surfaceindexfiu2CMmb2x8_fS_u2CMmr2x8_hS0_S0_()
; COM: Regression on incorrectly replaced instruction (%extractvalue78 here)
; COM: had its use in IR before its definition
; CHECK: %extractvalue78
; CHECK-NOT: %extractvalue78 =
  %extractvalue78 = extractvalue { <16 x i8>, <16 x i8>, <16 x i8> } %call, 2
  %zext130 = zext <16 x i8> %extractvalue78 to <16 x i32>
  %sub86 = sub nsw <16 x i32> %zext130, undef
  %absi87 = tail call <16 x i32> @llvm.genx.absi.v16i32(<16 x i32> %sub86)
  %.reduceintsize1828 = bitcast <16 x i32> %absi87 to <32 x i16>
  %.reduceintsize1829 = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> %.reduceintsize1828, i32 32, i32 16, i32 2, i16 0, i32 undef)
  %.reduceintsize_extend255.reduceintsize_extend = and <16 x i16> %.reduceintsize1829, <i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255, i16 255>
  %.reduceintsize_extend255.reduceintsize_extend.reduceintsize_extend = zext <16 x i16> %.reduceintsize_extend255.reduceintsize_extend to <16 x i32>
  %mul = mul <16 x i32> undef, %.reduceintsize_extend255.reduceintsize_extend.reduceintsize_extend
  %add89 = add <16 x i32> undef, %mul
  br label %1

1:                                                ; preds = %0, %._crit_edge
  ret void
}

declare <16 x i32> @llvm.genx.absi.v16i32(<16 x i32>) #0

declare <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16>, i32, i32, i32, i16, i32) #0

; Function Attrs: nounwind readnone
declare { <16 x i8>, <16 x i8>, <16 x i8> } @llvm.ssa.copy.sl_v16i8v16i8v16i8s({ <16 x i8>, <16 x i8>, <16 x i8> } returned) #2

attributes #0 = { "target-cpu"="Gen9" }
attributes #1 = { "CMGenxMain" "target-cpu"="Gen9" }
attributes #2 = { nounwind readnone "target-cpu"="Gen9" }

!genx.kernels = !{!0, !5}
!genx.kernel.internal = !{!10, !15}

!0 = !{void (i32, i32, i32, i32, i32, float, i64)* @analyze_motion_1, !"analyze_motion_1", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 2, i32 2, i32 2, i32 2, i32 2, i32 0, i32 96}
!2 = !{i32 72, i32 80, i32 -1, i32 -1, i32 -1, i32 88, i32 64}
!3 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!4 = !{!"buffer_t read_write", !"buffer_t read_write", !"image2d_t read_write", !"image2d_t read_write", !"image2d_t read_write", !""}
!5 = !{void (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, float, i64)* @analyze_motion_2, !"analyze_motion_2", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 0, i32 96}
!7 = !{i32 72, i32 80, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 88, i32 -1, i32 -1, i32 96, i32 64}
!8 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!9 = !{!"buffer_t read_write", !"buffer_t read_write", !"image2d_t read_write", !"image2d_t read_write", !"image2d_t read_write", !"image2d_t read_write", !"image2d_t read_write", !"image2d_t read_write", !"image2d_t read_write", !"buffer_t read_write", !"image2d_t read_write", !"image2d_t read_write", !""}
!10 = !{void (i32, i32, i32, i32, i32, float, i64)* @analyze_motion_1, !11, !12, !13, !14}
!11 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!12 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6}
!13 = !{}
!14 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 -1, i32 5}
!15 = !{void (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, float, i64)* @analyze_motion_2, !16, !17, !13, !18}
!16 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!17 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13}
!18 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 -1, i32 12}
