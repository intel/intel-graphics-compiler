;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-legalization -S < %s | FileCheck %s
; ------------------------------------------------
; Legalization: insertelement patterns
; ------------------------------------------------
; This test checks that Legalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'InsertElement.ll'
source_filename = "InsertElement.ll"

define spir_kernel void @test_insert(i32 %src1, <3 x i1> %src2) !dbg !7 {
; Testcase 1
; insertelement with const vector to several insertelements
; CHECK: [[INS1_V:%[0-9]*]] = insertelement <3 x i32> undef, i32 0, i32 0
; CHECK-NEXT: [[INS2_V:%[0-9]*]] = insertelement <3 x i32> [[INS1_V]], i32 1, i32 1
; CHECK-NEXT: [[INS3_V:%[0-9]*]] = insertelement <3 x i32> [[INS2_V]], i32 2, i32 2
; CHECK-NEXT: [[INS_EL_V:%[0-9]*]] = insertelement <3 x i32> [[INS3_V]], i32 %src1, i32 0, !dbg [[INS_EL_LOC:![0-9]*]]
; CHECK: [[DBG_VALUE_CALL:dbg.value\(metadata]] <3 x i32> [[INS_EL_V]],  metadata [[INS_EL_MD:![0-9]*]], {{.*}}, !dbg [[INS_EL_LOC]]

  %1 = insertelement <3 x i32> <i32 0, i32 1, i32 2>, i32 %src1, i32 0, !dbg !18
  call void @llvm.dbg.value(metadata <3 x i32> %1, metadata !10, metadata !DIExpression()), !dbg !18

; Testcase 2
; insertelement with i1 propagated to i32
; CHECK-NEXT: [[EXTR_V:%[0-9]*]] = extractelement <3 x i1> %src2, i32 2, !dbg [[EXTR_LOC:![0-9]*]]
; CHECK: [[SEXT_V:%[0-9]*]] = sext i1 [[EXTR_V]] to i32
; CHECK-NEXT: [[INS_EL32_V:%[0-9]*]] = insertelement <3 x i32> undef, i32 [[SEXT_V]], i32 1
; CHECK: [[EXTR32_V:%[0-9]*]] = extractelement <3 x i32> [[INS_EL32_V]], i32 1
; CHECK-NEXT: [[EXTR1_V:%[0-9]*]] = trunc i32 %10 to i1, !dbg [[EXTR1_LOC:![0-9]*]]
; CHECK: [[DBG_VALUE_CALL]] i1 [[EXTR1_V]], metadata [[EXTR1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR1_LOC]]

  %2 = extractelement <3 x i1> %src2, i32 2, !dbg !19
  call void @llvm.dbg.value(metadata i1 %2, metadata !12, metadata !DIExpression()), !dbg !19
  %3 = insertelement <3 x i1> %src2, i1 %2, i32 1, !dbg !20
  call void @llvm.dbg.value(metadata <3 x i1> %3, metadata !14, metadata !DIExpression()), !dbg !20
  %4 = extractelement <3 x i1> %3, i32 1, !dbg !21
  call void @llvm.dbg.value(metadata i1 %4, metadata !16, metadata !DIExpression()), !dbg !21
  %5 = select i1 %4, i32 13, i32 14, !dbg !22
  call void @llvm.dbg.value(metadata i32 %5, metadata !17, metadata !DIExpression()), !dbg !22
  ret void, !dbg !23
}

; Testcase 1 MD:
; CHECK-DAG: [[INS_EL_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[INS_EL_MD]] = !DILocalVariable(name: "1"

; Testcase 2 MD:
; CHECK-DAG: [[EXTR_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[EXTR1_LOC]] = !DILocation(line: 4
; CHECK-DAG: [[EXTR1_MD]] = !DILocalVariable(name: "4"

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!2}
!llvm.debugify = !{!4, !5}
!llvm.module.flags = !{!6}

!0 = !{void (i32, <3 x i1>)* @test_insert, !1}
!1 = !{}
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !1)
!3 = !DIFile(filename: "InsertElement.ll", directory: "/")
!4 = !{i32 6}
!5 = !{i32 5}
!6 = !{i32 2, !"Debug Info Version", i32 3}
!7 = distinct !DISubprogram(name: "test_insert", linkageName: "test_insert", scope: null, file: !3, line: 1, type: !8, scopeLine: 1, unit: !2, retainedNodes: !9)
!8 = !DISubroutineType(types: !1)
!9 = !{!10, !12, !14, !16, !17}
!10 = !DILocalVariable(name: "1", scope: !7, file: !3, line: 1, type: !11)
!11 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!12 = !DILocalVariable(name: "2", scope: !7, file: !3, line: 2, type: !13)
!13 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "3", scope: !7, file: !3, line: 3, type: !15)
!15 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "4", scope: !7, file: !3, line: 4, type: !13)
!17 = !DILocalVariable(name: "5", scope: !7, file: !3, line: 5, type: !15)
!18 = !DILocation(line: 1, column: 1, scope: !7)
!19 = !DILocation(line: 2, column: 1, scope: !7)
!20 = !DILocation(line: 3, column: 1, scope: !7)
!21 = !DILocation(line: 4, column: 1, scope: !7)
!22 = !DILocation(line: 5, column: 1, scope: !7)
!23 = !DILocation(line: 6, column: 1, scope: !7)
