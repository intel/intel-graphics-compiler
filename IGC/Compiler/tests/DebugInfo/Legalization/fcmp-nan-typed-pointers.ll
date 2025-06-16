;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-legalization --preserve-nan=1 -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: fcmp patterns with preserve NaN
; ------------------------------------------------
; This test checks that Legalization pass follows
; 'How to Update Debug Info' llvm guideline.

; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'FCmpNaN.ll'
source_filename = "FCmpNaN.ll"

define spir_kernel void @test_fcmp(float %src1, float %src2) !dbg !7 {
; NaN check is performed separately and then logically or'd or and'd

; Testcase 1
; cmp ord to and(oeq,oeq)
; CHECK-DAG:  [[FCMP_OEQ2:%[0-9]*]] = fcmp oeq float %src2, %src2
; CHECK-DAG:  [[FCMP_OEQ1:%[0-9]*]] = fcmp oeq float %src1, %src1
; CHECK:      [[FCMP_ORD_V:%[0-9]*]] = and i1 [[FCMP_OEQ1]], [[FCMP_OEQ2]], !dbg [[FCMP_ORD_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i1 [[FCMP_ORD_V]],  metadata [[FCMP_ORD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_ORD_LOC]]
; CHECK-NEXT: [[ZEXT_ORD_V:%[0-9]*]] = zext i1 [[FCMP_ORD_V]] to i32, !dbg [[ZEXT_ORD_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i32 [[ZEXT_ORD_V]],  metadata [[ZEXT_ORD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ZEXT_ORD_LOC]]

  %1 = fcmp ord float %src1, %src2, !dbg !23
  call void @llvm.dbg.value(metadata i1 %1, metadata !10, metadata !DIExpression()), !dbg !23
  %2 = zext i1 %1 to i32, !dbg !24
  call void @llvm.dbg.value(metadata i32 %2, metadata !12, metadata !DIExpression()), !dbg !24

; Testcase 2
; cmp uno to or(une,une)
; CHECK-DAG:  [[FCMP_UNE1:%[0-9]*]] = fcmp une float %src1, %src1
; CHECK-DAG:  [[FCMP_UNE2:%[0-9]*]] = fcmp une float %src2, %src2
; CHECK:      [[FCMP_UNO_V:%[0-9]*]] = or i1 [[FCMP_UNE1]], [[FCMP_UNE2]], !dbg [[FCMP_UNO_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i1 [[FCMP_UNO_V]],  metadata [[FCMP_UNO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_UNO_LOC]]
; CHECK-NEXT: [[ZEXT_UNO_V:%[0-9]*]] = zext i1 [[FCMP_UNO_V]] to i32, !dbg [[ZEXT_UNO_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i32 [[ZEXT_UNO_V]],  metadata [[ZEXT_UNO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ZEXT_UNO_LOC]]

  %3 = fcmp uno float %src1, %src2, !dbg !25
  call void @llvm.dbg.value(metadata i1 %3, metadata !14, metadata !DIExpression()), !dbg !25
  %4 = zext i1 %3 to i32, !dbg !26
  call void @llvm.dbg.value(metadata i32 %4, metadata !15, metadata !DIExpression()), !dbg !26

; Testcase 3
; cmp one to and(une, and(oeq,oeq))

; CHECK-DAG:  [[FCMP_OEQ2:%[0-9]*]] = fcmp oeq float %src2, %src2
; CHECK-DAG:  [[FCMP_OEQ1:%[0-9]*]] = fcmp oeq float %src1, %src1
; CHECK:      [[FCMP_ORD_V:%[0-9]*]] = and i1 [[FCMP_OEQ1]], [[FCMP_OEQ2]]
; CHECK-NEXT: [[FCMP_UNE_V:%[0-9]*]] = fcmp une float %src1, %src2
; CHECK-NEXT: [[FCMP_ONE_V:%[0-9]*]] = and i1 [[FCMP_ORD_V]], [[FCMP_UNE_V]], !dbg [[FCMP_ONE_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i1 [[FCMP_ONE_V]],  metadata [[FCMP_ONE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_ONE_LOC]]
; CHECK-NEXT: [[ZEXT_ONE_V:%[0-9]*]] = zext i1 [[FCMP_ONE_V]] to i32, !dbg [[ZEXT_ONE_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i32 [[ZEXT_ONE_V]],  metadata [[ZEXT_ONE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ZEXT_ONE_LOC]]

  %5 = fcmp one float %src1, %src2, !dbg !27
  call void @llvm.dbg.value(metadata i1 %5, metadata !16, metadata !DIExpression()), !dbg !27
  %6 = zext i1 %5 to i32, !dbg !28
  call void @llvm.dbg.value(metadata i32 %6, metadata !17, metadata !DIExpression()), !dbg !28

; Testcase 4
; cmp ueq to or(oeq, or(une,une))
; CHECK-DAG:  [[FCMP_UNE1:%[0-9]*]] = fcmp une float %src1, %src1
; CHECK-DAG:  [[FCMP_UNE2:%[0-9]*]] = fcmp une float %src2, %src2
; CHECK:      [[FCMP_UNO_V:%[0-9]*]] = or i1 [[FCMP_UNE1]], [[FCMP_UNE2]]
; CHECK-NEXT: [[FCMP_OEQ_V:%[0-9]*]] = fcmp oeq float %src1, %src2
; CHECK-NEXT: [[FCMP_UEQ_V:%[0-9]*]] = or i1 [[FCMP_UNO_V]], [[FCMP_OEQ_V]], !dbg [[FCMP_UEQ_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i1 [[FCMP_UEQ_V]],  metadata [[FCMP_UEQ_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_UEQ_LOC]]
; CHECK-NEXT: [[ZEXT_UEQ_V:%[0-9]*]] = zext i1 [[FCMP_UEQ_V]] to i32, !dbg [[ZEXT_UEQ_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i32 [[ZEXT_UEQ_V]],  metadata [[ZEXT_UEQ_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ZEXT_UEQ_LOC]]

  %7 = fcmp ueq float %src1, %src2, !dbg !29
  call void @llvm.dbg.value(metadata i1 %7, metadata !18, metadata !DIExpression()), !dbg !29
  %8 = zext i1 %7 to i32, !dbg !30
  call void @llvm.dbg.value(metadata i32 %8, metadata !19, metadata !DIExpression()), !dbg !30

; Testcase 5
; cmp uge to !olt

; CHECK-NEXT: [[FCMP_OLT:%[0-9]*]] = fcmp olt float %src1, %src2
; CHECK-NEXT: [[FCMP_UGE_V:%[0-9]*]] = xor i1 [[FCMP_OLT]], true, !dbg [[FCMP_UGE_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i1 [[FCMP_UGE_V]],  metadata [[FCMP_UGE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_UGE_LOC]]
; CHECK-NEXT: [[ZEXT_UGE_V:%[0-9]*]] = zext i1 [[FCMP_UGE_V]] to i32, !dbg [[ZEXT_UGE_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] i32 [[ZEXT_UGE_V]],  metadata [[ZEXT_UGE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ZEXT_UGE_LOC]]

  %9 = fcmp uge float %src1, %src2, !dbg !31
  call void @llvm.dbg.value(metadata i1 %9, metadata !20, metadata !DIExpression()), !dbg !31
  %10 = zext i1 %9 to i32, !dbg !32
  call void @llvm.dbg.value(metadata i32 %10, metadata !21, metadata !DIExpression()), !dbg !32
  %11 = select i1 %9, i32 1, i32 0, !dbg !33
  call void @llvm.dbg.value(metadata i32 %11, metadata !22, metadata !DIExpression()), !dbg !33
  ret void, !dbg !34
}

; Testcase 1 MD:
; CHECK-DAG: [[FCMP_ORD_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[ZEXT_ORD_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[FCMP_ORD_MD]] = !DILocalVariable(name: "1", scope: !7, file: !3, line: 1, type: !11)
; CHECK-DAG: [[ZEXT_ORD_MD]] = !DILocalVariable(name: "2", scope: !7, file: !3, line: 2, type: !13)

; Testcase 2 MD:
; CHECK-DAG: [[FCMP_UNO_LOC]] = !DILocation(line: 3
; CHECK-DAG: [[ZEXT_UNO_LOC]] = !DILocation(line: 4
; CHECK-DAG: [[FCMP_UNO_MD]] = !DILocalVariable(name: "3", scope: !7, file: !3, line: 3, type: !11)
; CHECK-DAG: [[ZEXT_UNO_MD]] = !DILocalVariable(name: "4", scope: !7, file: !3, line: 4, type: !13)

; Testcase 3 MD:

; CHECK-DAG: [[FCMP_ONE_LOC]] = !DILocation(line: 5
; CHECK-DAG: [[ZEXT_ONE_LOC]] = !DILocation(line: 6
; CHECK-DAG: [[FCMP_ONE_MD]] = !DILocalVariable(name: "5", scope: !7, file: !3, line: 5, type: !11)
; CHECK-DAG: [[ZEXT_ONE_MD]] = !DILocalVariable(name: "6", scope: !7, file: !3, line: 6, type: !13)

; Testcase 4 MD:
; CHECK-DAG: [[FCMP_UEQ_LOC]] = !DILocation(line: 7
; CHECK-DAG: [[ZEXT_UEQ_LOC]] = !DILocation(line: 8
; CHECK-DAG: [[FCMP_UEQ_MD]] = !DILocalVariable(name: "7", scope: !7, file: !3, line: 7, type: !11)
; CHECK-DAG: [[ZEXT_UEQ_MD]] = !DILocalVariable(name: "8", scope: !7, file: !3, line: 8, type: !13)

; Testcase 5 MD:
; CHECK-DAG: [[FCMP_UGE_LOC]] = !DILocation(line: 9
; CHECK-DAG: [[ZEXT_UGE_LOC]] = !DILocation(line: 10
; CHECK-DAG: [[FCMP_UGE_MD]] = !DILocalVariable(name: "9", scope: !7, file: !3, line: 9, type: !11)
; CHECK-DAG: [[ZEXT_UGE_MD]] = !DILocalVariable(name: "10", scope: !7, file: !3, line: 10, type: !13)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!2}
!llvm.debugify = !{!4, !5}
!llvm.module.flags = !{!6}

!0 = !{void (float, float)* @test_fcmp, !1}
!1 = !{}
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !1)
!3 = !DIFile(filename: "FCmpNaN.ll", directory: "/")
!4 = !{i32 12}
!5 = !{i32 11}
!6 = !{i32 2, !"Debug Info Version", i32 3}
!7 = distinct !DISubprogram(name: "test_fcmp", linkageName: "test_fcmp", scope: null, file: !3, line: 1, type: !8, scopeLine: 1, unit: !2, retainedNodes: !9)
!8 = !DISubroutineType(types: !1)
!9 = !{!10, !12, !14, !15, !16, !17, !18, !19, !20, !21, !22}
!10 = !DILocalVariable(name: "1", scope: !7, file: !3, line: 1, type: !11)
!11 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!12 = !DILocalVariable(name: "2", scope: !7, file: !3, line: 2, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "3", scope: !7, file: !3, line: 3, type: !11)
!15 = !DILocalVariable(name: "4", scope: !7, file: !3, line: 4, type: !13)
!16 = !DILocalVariable(name: "5", scope: !7, file: !3, line: 5, type: !11)
!17 = !DILocalVariable(name: "6", scope: !7, file: !3, line: 6, type: !13)
!18 = !DILocalVariable(name: "7", scope: !7, file: !3, line: 7, type: !11)
!19 = !DILocalVariable(name: "8", scope: !7, file: !3, line: 8, type: !13)
!20 = !DILocalVariable(name: "9", scope: !7, file: !3, line: 9, type: !11)
!21 = !DILocalVariable(name: "10", scope: !7, file: !3, line: 10, type: !13)
!22 = !DILocalVariable(name: "11", scope: !7, file: !3, line: 11, type: !13)
!23 = !DILocation(line: 1, column: 1, scope: !7)
!24 = !DILocation(line: 2, column: 1, scope: !7)
!25 = !DILocation(line: 3, column: 1, scope: !7)
!26 = !DILocation(line: 4, column: 1, scope: !7)
!27 = !DILocation(line: 5, column: 1, scope: !7)
!28 = !DILocation(line: 6, column: 1, scope: !7)
!29 = !DILocation(line: 7, column: 1, scope: !7)
!30 = !DILocation(line: 8, column: 1, scope: !7)
!31 = !DILocation(line: 9, column: 1, scope: !7)
!32 = !DILocation(line: 10, column: 1, scope: !7)
!33 = !DILocation(line: 11, column: 1, scope: !7)
!34 = !DILocation(line: 12, column: 1, scope: !7)
