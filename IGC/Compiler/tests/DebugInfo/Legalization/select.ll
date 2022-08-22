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
; Legalization: select patterns
; ------------------------------------------------
; This test checks that Legalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'Select.ll'
source_filename = "Select.ll"

define spir_kernel void @test_select(i64 %src1, i64 %src2, i1 %src3, i1 %src4) !dbg !7 {
; Testcase 1
;
; CHECK: [[ZEXT1_V:%[0-9]*]] = zext i1 %src3 to i32
; CHECK-NEXT: [[ZEXT2_V:%[0-9]*]] = zext i1 %src4 to i32
; CHECK-NEXT: [[SELECT32_V:%[0-9]*]] = select i1 [[COND_V:%[0-9]*]], i32 [[ZEXT1_V]], i32 [[ZEXT2_V]]
; CHECK-NEXT: [[SELECT_V:%[0-9]*]] = trunc i32 [[SELECT32_V]] to i1, !dbg [[SELECT1_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] i1 [[SELECT_V]],  metadata [[SELECT1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT1_LOC]]

  %1 = icmp slt i64 %src1, %src2, !dbg !20
  call void @llvm.dbg.value(metadata i1 %1, metadata !10, metadata !DIExpression()), !dbg !20
  %2 = select i1 %1, i1 %src3, i1 %src4, !dbg !21
  call void @llvm.dbg.value(metadata i1 %2, metadata !12, metadata !DIExpression()), !dbg !21

; Testcase 2:
; CHECK: [[EXTR10_V:%[0-9]*]] = extractelement <2 x i64> [[VEC1:%[0-9]*]], i32 0
; CHECK-NEXT: [[EXTR20_V:%[0-9]*]] = extractelement <2 x i64> [[VEC2:%[0-9]*]], i32 0
; CHECK-NEXT: [[SELECT0_V:%[0-9]*]] = select i1 [[SELECT_V]], i64 [[EXTR10_V]], i64 [[EXTR20_V]]
; CHECK-NEXT: [[INS_EL0_V:%[0-9]*]] = insertelement <2 x i64> undef, i64 [[SELECT0_V]], i32 0
; CHECK-NEXT: [[EXTR11_V:%[0-9]*]] = extractelement <2 x i64> [[VEC1]], i32 1
; CHECK-NEXT: [[EXTR21_V:%[0-9]*]] = extractelement <2 x i64> [[VEC2]], i32 1
; CHECK-NEXT: [[SELECT1_V:%[0-9]*]] = select i1 [[SELECT_V]], i64 [[EXTR11_V]], i64 [[EXTR21_V]]
; CHECK-NEXT: [[SELECTV_V:%[0-9]*]] = insertelement <2 x i64> [[INS_EL0_V]], i64 [[SELECT1_V]], i32 1, !dbg [[SELECTV_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] <2 x i64> [[SELECTV_V]], metadata [[SELECTV_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECTV_LOC]]

  %3 = insertelement <2 x i64> zeroinitializer, i64 %src1, i32 0, !dbg !22
  call void @llvm.dbg.value(metadata <2 x i64> %3, metadata !13, metadata !DIExpression()), !dbg !22
  %4 = insertelement <2 x i64> zeroinitializer, i64 %src2, i32 1, !dbg !23
  call void @llvm.dbg.value(metadata <2 x i64> %4, metadata !15, metadata !DIExpression()), !dbg !23
  %5 = select i1 %2, <2 x i64> %3, <2 x i64> %4, !dbg !24
  call void @llvm.dbg.value(metadata <2 x i64> %5, metadata !16, metadata !DIExpression()), !dbg !24

; Testcase 3:
; CHECK: [[COND0_V:%[0-9]*]] = extractelement <2 x i1> [[CONDV_V:%[0-9]*]], i32 0
; CHECK: [[SELECT0_V:%[0-9]*]] = select i1 [[COND0_V]]
; CHECK: [[COND1_V:%[0-9]*]] = extractelement <2 x i1> [[CONDV_V]], i32 1
; CHECK: [[SELECT1_V:%[0-9]*]] = select i1 [[COND1_V]]
; CHECK: [[SELECTV2_V:%[0-9]*]] = insertelement <2 x i64> {{.*}}, i64 [[SELECT1_V]], i32 1, !dbg [[SELECTV2_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] <2 x i64> [[SELECTV2_V]], metadata [[SELECTV2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECTV2_LOC]]

  %6 = insertelement <2 x i1> <i1 false, i1 true>, i1 %1, i32 0, !dbg !25
  call void @llvm.dbg.value(metadata <2 x i1> %6, metadata !17, metadata !DIExpression()), !dbg !25
  %7 = select <2 x i1> %6, <2 x i64> %3, <2 x i64> %4, !dbg !26
  call void @llvm.dbg.value(metadata <2 x i64> %7, metadata !19, metadata !DIExpression()), !dbg !26
  ret void, !dbg !27
}

; Testcase 1 MD:
; CHECK-DAG: [[SELECT1_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[SELECT1_MD]] = !DILocalVariable(name: "2"
; Testcase 2 MD:
; CHECK-DAG: [[SELECTV_MD]] = !DILocalVariable(name: "5"
; CHECK-DAG: [[SELECTV_LOC]] = !DILocation(line: 5
; Testcase 3 MD:
; CHECK-DAG: [[SELECTV2_MD]] = !DILocalVariable(name: "7"
; CHECK-DAG: [[SELECTV2_LOC]] = !DILocation(line: 7

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!2}
!llvm.debugify = !{!4, !5}
!llvm.module.flags = !{!6}

!0 = !{void (i64, i64, i1, i1)* @test_select, !1}
!1 = !{}
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !1)
!3 = !DIFile(filename: "Select.ll", directory: "/")
!4 = !{i32 8}
!5 = !{i32 7}
!6 = !{i32 2, !"Debug Info Version", i32 3}
!7 = distinct !DISubprogram(name: "test_select", linkageName: "test_select", scope: null, file: !3, line: 1, type: !8, scopeLine: 1, unit: !2, retainedNodes: !9)
!8 = !DISubroutineType(types: !1)
!9 = !{!10, !12, !13, !15, !16, !17, !19}
!10 = !DILocalVariable(name: "1", scope: !7, file: !3, line: 1, type: !11)
!11 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!12 = !DILocalVariable(name: "2", scope: !7, file: !3, line: 2, type: !11)
!13 = !DILocalVariable(name: "3", scope: !7, file: !3, line: 3, type: !14)
!14 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !7, file: !3, line: 4, type: !14)
!16 = !DILocalVariable(name: "5", scope: !7, file: !3, line: 5, type: !14)
!17 = !DILocalVariable(name: "6", scope: !7, file: !3, line: 6, type: !18)
!18 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!19 = !DILocalVariable(name: "7", scope: !7, file: !3, line: 7, type: !14)
!20 = !DILocation(line: 1, column: 1, scope: !7)
!21 = !DILocation(line: 2, column: 1, scope: !7)
!22 = !DILocation(line: 3, column: 1, scope: !7)
!23 = !DILocation(line: 4, column: 1, scope: !7)
!24 = !DILocation(line: 5, column: 1, scope: !7)
!25 = !DILocation(line: 6, column: 1, scope: !7)
!26 = !DILocation(line: 7, column: 1, scope: !7)
!27 = !DILocation(line: 8, column: 1, scope: !7)
