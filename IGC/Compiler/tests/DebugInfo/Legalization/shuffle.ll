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
; Legalization: shufflevector patterns
; ------------------------------------------------
; This test checks that Legalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'ShuffleVector.ll'
source_filename = "ShuffleVector.ll"

define spir_kernel void @test_shuffle(<4 x i32> %src1, <4 x i32> %src2) !dbg !7 {

; Testcase 1 shuffle vector with non const vectors
;
; CHECK: [[EXTR0_V:%[0-9]*]] = extractelement <4 x i32> %src1, i32 0
; CHECK-NEXT: [[INS0_V:%[0-9]*]] = insertelement <6 x i32> undef, i32 [[EXTR0_V]], i32 0
; CHECK-NEXT: [[EXTR1_V:%[0-9]*]] = extractelement <4 x i32> %src1, i32 1
; CHECK-NEXT: [[INS1_V:%[0-9]*]] = insertelement <6 x i32> [[INS0_V]], i32 [[EXTR1_V]], i32 1
; CHECK-NEXT: [[EXTR2_V:%[0-9]*]] = extractelement <4 x i32> %src1, i32 3
; CHECK-NEXT: [[INS2_V:%[0-9]*]] = insertelement <6 x i32> [[INS1_V]], i32 [[EXTR2_V]], i32 2
; CHECK-NEXT: [[EXTR3_V:%[0-9]*]] = extractelement <4 x i32> %src2, i32 0
; CHECK-NEXT: [[INS3_V:%[0-9]*]] = insertelement <6 x i32> [[INS2_V]], i32 [[EXTR3_V]], i32 3
; CHECK-NEXT: [[EXTR4_V:%[0-9]*]] = extractelement <4 x i32> %src2, i32 1
; CHECK-NEXT: [[INS4_V:%[0-9]*]] = insertelement <6 x i32> [[INS3_V]], i32 [[EXTR4_V]], i32 4
; CHECK-NEXT: [[EXTR5_V:%[0-9]*]] = extractelement <4 x i32> %src2, i32 2
; CHECK-NEXT: [[SHUF_V:%[0-9]*]] = insertelement <6 x i32> [[INS4_V]], i32 [[EXTR5_V]], i32 5, !dbg [[SHUF_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] <6 x i32> [[SHUF_V]],  metadata [[SHUF_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHUF_LOC]]

  %1 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <6 x i32> <i32 0, i32 1, i32 3, i32 4, i32 5, i32 6>, !dbg !16
  call void @llvm.dbg.value(metadata <6 x i32> %1, metadata !10, metadata !DIExpression()), !dbg !16

; Testcase 2 shuffle vector with const vector
;
; CHECK-NEXT: [[INS0_V:%[0-9]*]] = insertelement <6 x i32> undef, i32 1, i32 0
; CHECK-NEXT: [[INS1_V:%[0-9]*]] = insertelement <6 x i32> [[INS0_V]], i32 0, i32 1
; CHECK-NEXT: [[INS2_V:%[0-9]*]] = insertelement <6 x i32> [[INS1_V]], i32 4, i32 2
; CHECK-NEXT: [[INS3_V:%[0-9]*]] = insertelement <6 x i32> [[INS2_V]], i32 42, i32 3
; CHECK-NEXT: [[EXTR3_V:%[0-9]*]] = extractelement <4 x i32> %src1, i32 3
; CHECK-NEXT: [[INS4_V:%[0-9]*]] = insertelement <6 x i32> [[INS3_V]], i32 [[EXTR3_V]], i32 4
; CHECK-NEXT: [[EXTR2_V:%[0-9]*]] = extractelement <4 x i32> %src1, i32 2
; CHECK-NEXT: [[SHUF1_V:%[0-9]*]] = insertelement <6 x i32> [[INS4_V]], i32 [[EXTR2_V]], i32 5, !dbg [[SHUF1_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] <6 x i32> [[SHUF1_V]],  metadata [[SHUF1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHUF1_LOC]]

  %2 = shufflevector <4 x i32> %src1, <4 x i32> <i32 42, i32 0, i32 1, i32 4>, <6 x i32> <i32 6, i32 5, i32 7, i32 4, i32 3, i32 2>, !dbg !17
  call void @llvm.dbg.value(metadata <6 x i32> %2, metadata !12, metadata !DIExpression()), !dbg !17

; Testcase 3 shuffle vector with undef mask
;
; CHECK-NEXT: [[EXTR0_V:%[0-9]*]] = extractelement <4 x i32> %src2, i32 0
; CHECK-NEXT: [[INS1_V:%[0-9]*]] = insertelement <4 x i32> undef, i32 [[EXTR0_V]], i32 1
; CHECK-NEXT: [[EXTR2_V:%[0-9]*]] = extractelement <4 x i32> %src1, i32 2
; CHECK-NEXT: [[SHUF2_V:%[0-9]*]] = insertelement <4 x i32> [[INS1_V]], i32 [[EXTR2_V]], i32 3, !dbg [[SHUF2_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] <4 x i32> [[SHUF2_V]],  metadata [[SHUF2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHUF2_LOC]]

  %3 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <4 x i32> <i32 undef, i32 4, i32 undef, i32 2>, !dbg !18
  call void @llvm.dbg.value(metadata <4 x i32> %3, metadata !13, metadata !DIExpression()), !dbg !18

; Testcase 4 reuse source vector
;
; CHECK-NEXT: [[EXTR0_V:%[0-9]*]] = extractelement <4 x i32> %src2, i32 0
; CHECK-NEXT: [[INS2_V:%[0-9]*]] = insertelement <4 x i32> %src1, i32 [[EXTR0_V]], i32 2
; CHECK-NEXT: [[EXTR1_V:%[0-9]*]] = extractelement <4 x i32> %src2, i32 1
; CHECK-NEXT: [[SHUF3_V:%[0-9]*]] = insertelement <4 x i32> [[INS2_V]], i32 [[EXTR1_V]], i32 3, !dbg [[SHUF3_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL]] <4 x i32> [[SHUF3_V]],  metadata [[SHUF3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHUF3_LOC]]

  %4 = shufflevector <4 x i32> %src1, <4 x i32> %src2, <4 x i32> <i32 0, i32 1, i32 4, i32 5>, !dbg !19
  call void @llvm.dbg.value(metadata <4 x i32> %4, metadata !15, metadata !DIExpression()), !dbg !19
  ret void, !dbg !20
}

; Testcase 1 MD:
; CHECK-DAG: [[SHUF_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[SHUF_MD]] = !DILocalVariable(name: "1"
; Testcase 2 MD:
; CHECK-DAG: [[SHUF1_LOC]] = !DILocation(line: 2
; CHECK-DAG: [[SHUF1_MD]] = !DILocalVariable(name: "2"
; Testcase 3 MD:
; CHECK-DAG: [[SHUF2_LOC]] = !DILocation(line: 3
; CHECK-DAG: [[SHUF2_MD]] = !DILocalVariable(name: "3"
; Testcase 4 MD:
; CHECK-DAG: [[SHUF3_LOC]] = !DILocation(line: 4
; CHECK-DAG: [[SHUF3_MD]] = !DILocalVariable(name: "4"


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!2}
!llvm.debugify = !{!4, !5}
!llvm.module.flags = !{!6}

!0 = !{void (<4 x i32>, <4 x i32>)* @test_shuffle, !1}
!1 = !{}
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !1)
!3 = !DIFile(filename: "ShuffleVector.ll", directory: "/")
!4 = !{i32 5}
!5 = !{i32 4}
!6 = !{i32 2, !"Debug Info Version", i32 3}
!7 = distinct !DISubprogram(name: "test_shuffle", linkageName: "test_shuffle", scope: null, file: !3, line: 1, type: !8, scopeLine: 1, unit: !2, retainedNodes: !9)
!8 = !DISubroutineType(types: !1)
!9 = !{!10, !12, !13, !15}
!10 = !DILocalVariable(name: "1", scope: !7, file: !3, line: 1, type: !11)
!11 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!12 = !DILocalVariable(name: "2", scope: !7, file: !3, line: 2, type: !11)
!13 = !DILocalVariable(name: "3", scope: !7, file: !3, line: 3, type: !14)
!14 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !7, file: !3, line: 4, type: !14)
!16 = !DILocation(line: 1, column: 1, scope: !7)
!17 = !DILocation(line: 2, column: 1, scope: !7)
!18 = !DILocation(line: 3, column: 1, scope: !7)
!19 = !DILocation(line: 4, column: 1, scope: !7)
!20 = !DILocation(line: 5, column: 1, scope: !7)
