;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --fix-fast-math-flags -S < %s | FileCheck %s
; ------------------------------------------------
; FixFastMathFlags
; ------------------------------------------------
; This test checks that FixFastMathFlags pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; This pass only modifies flags on fp instructions
; ------------------------------------------------


; ModuleID = 'FixFastMathFlags.ll'
source_filename = "FixFastMathFlags.ll"

define spir_kernel void @test_fpflags(float %s1, float %s2) !dbg !6 {
; Flags modified
; CHECK: [[FADD_V:%[0-9]*]] = fadd
; CHECK-DAG: reassoc
; CHECK-DAG: nsz
; CHECK-DAG: afn
; CHECK-DAG: contract
; CHECK-SAME: !dbg [[FADD_LOC:![0-9]*]]
; CHECK: dbg.value
; CHECK-SAME: float [[FADD_V]], metadata [[FADD_MD:![0-9]*]]
; CHECK_SAME: !dbg [[FADD_LOC]]
  %1 = fadd reassoc nsz float %s1, %s2, !dbg !15
  call void @llvm.dbg.value(metadata float %1, metadata !9, metadata !DIExpression()), !dbg !15
; Flags not modified
; CHECK: [[FDIV_V:%[0-9]*]] = fdiv arcp float
; CHECK-SAME: !dbg [[FDIV_LOC:![0-9]*]]
; CHECK: dbg.value
; CHECK-SAME: float [[FDIV_V]], metadata [[FDIV_MD:![0-9]*]]
; CHECK_SAME: !dbg [[FDIV_LOC]]
  %2 = fdiv arcp float %s1, %s2, !dbg !16
  call void @llvm.dbg.value(metadata float %2, metadata !11, metadata !DIExpression()), !dbg !16
; Flags discarded
; CHECK: [[FCMPO_V:%[0-9]*]] = fcmp uno float
; CHECK-SAME: !dbg [[FCMPO_LOC:![0-9]*]]
; CHECK: dbg.value
; CHECK-SAME: i1 [[FCMPO_V]], metadata [[FCMPO_MD:![0-9]*]]
; CHECK_SAME: !dbg [[FCMPO_LOC]]
  %3 = fcmp ninf uno float %1, 0.000000e+00, !dbg !17
  call void @llvm.dbg.value(metadata i1 %3, metadata !12, metadata !DIExpression()), !dbg !17
; Flags discarded
; CHECK: [[FCMPNE_V:%[0-9]*]] = fcmp une float
; CHECK-SAME: !dbg [[FCMPNE_LOC:![0-9]*]]
; CHECK: dbg.value
; CHECK-SAME: i1 [[FCMPNE_V]], metadata [[FCMPNE_MD:![0-9]*]]
; CHECK_SAME: !dbg [[FCMPO_LOC]]
  %4 = fcmp fast une float %2, %2, !dbg !18
  call void @llvm.dbg.value(metadata i1 %4, metadata !14, metadata !DIExpression()), !dbg !18
  ret void, !dbg !19
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "FixFastMathFlags.ll"
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_fpflags", linkageName: "test_fpflags", scope: null, file: [[FILE]], line: 1
;
; CHECK-DAG: [[FADD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FADD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
;
; CHECK-DAG: [[FDIV_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FDIV_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
;
; CHECK-DAG: [[FCMPO_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FCMPO_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
;
; CHECK-DAG: [[FCMPNE_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FCMPNE_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "FixFastMathFlags.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_fpflags", linkageName: "test_fpflags", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !13)
!15 = !DILocation(line: 1, column: 1, scope: !6)
!16 = !DILocation(line: 2, column: 1, scope: !6)
!17 = !DILocation(line: 3, column: 1, scope: !6)
!18 = !DILocation(line: 4, column: 1, scope: !6)
!19 = !DILocation(line: 5, column: 1, scope: !6)
