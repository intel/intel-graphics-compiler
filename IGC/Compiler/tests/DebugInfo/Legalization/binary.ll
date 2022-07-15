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
; Legalization: frem, Or and And patterns
; ------------------------------------------------
; This test checks that Legalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'BinaryOperators.ll'
source_filename = "BinaryOperators.ll"

define spir_kernel void @test_binary(float %src1, float %src2, i1 %src3, i1 %src4) !dbg !7 {
entry:
; Testcase 1 frem transformation
; CHECK: [[FMUL:%[0-9]*]] = fmul float %src1, %src2
; CHECK-NEXT: [[FNEG1:%[0-9]*]] = {{fneg|fsub}} {{.*}} [[FMUL]]
; CHECK-NEXT: [[FCMP:%[0-9]*]] = fcmp oge float [[FMUL]], [[FNEG1]]
; CHECK-NEXT: [[FNEG2:%[0-9]*]] = {{fneg|fsub}} {{.*}} %src2
; CHECK-NEXT: [[SELECT:%[0-9]*]] = select i1 [[FCMP]], float %src2, float [[FNEG2]]
; CHECK-NEXT: [[FDIV:%[0-9]*]] = fdiv float 1.000000e+00, [[SELECT]]
; CHECK-NEXT: [[FMUL:%[0-9]*]] = fmul float %src1, [[FDIV]]
; CHECK-NEXT: [[FLOOR:%[0-9]*]] = call float @llvm.floor.f32(float [[FMUL]])
; CHECK-NEXT: [[FSUB:%[0-9]*]] = fsub float [[FMUL]], [[FLOOR]]
; CHECK-NEXT: [[FREM_V:%[0-9]*]] = fmul float [[FSUB]], [[SELECT]], !dbg [[FREM_LOC:![0-9]*]]
; CHECK-NEXT: [[DBG_VALUE_CALL:dbg.value\(metadata]] float [[FREM_V]],  metadata [[FREM_MD:![0-9]*]], {{.*}}, !dbg [[FREM_LOC]]
  %0 = frem float %src1, %src2, !dbg !21
  call void @llvm.dbg.value(metadata float %0, metadata !10, metadata !DIExpression()), !dbg !21

; Testcase 2-3 And & Or trasnformations
; Only select and branch lines should be preseved
; Value can be salvaged(not checked) and not salvaged FIXME
; CHECK: [[NOT_OR:%[0-9]*]] = and i1 %src3, %src4
; CHECK-NEXT: [[DBG_VALUE_CALL]] {{.*}},  metadata [[OR_MD:![0-9]*]], {{.*}}, !dbg [[OR_LOC:![0-9]*]]
; CHECK-NEXT: [[NOT_AND:%[0-9]*]] = or i1 %src3, %src4
; CHECK-NEXT: [[DBG_VALUE_CALL]] {{.*}},  metadata [[AND_MD:![0-9]*]], {{.*}}, !dbg [[AND_LOC:![0-9]*]]
; CHECK-NEXT: br i1 [[NOT_OR]], label %then.or, label %endif.or, !dbg [[BRANCH_OR_LOC:![0-9]*]]
; CHECK: %select.or = select i1 [[NOT_OR]], float %src2, float %src1, !dbg [[SELECT_OR_LOC:![0-9]*]]
; CHECK: br i1 [[NOT_AND]], label %then.and, label %endif.and, !dbg [[BRANCH_AND_LOC:![0-9]*]]
; CHECK: %select.and = select i1 [[NOT_AND]], float %src2, float %src1, !dbg [[SELECT_AND_LOC:![0-9]*]]
  %1 = xor i1 %src3, true, !dbg !22
  call void @llvm.dbg.value(metadata i1 %1, metadata !12, metadata !DIExpression()), !dbg !22
  %2 = xor i1 %src4, true, !dbg !23
  call void @llvm.dbg.value(metadata i1 %2, metadata !14, metadata !DIExpression()), !dbg !23
  %3 = xor i1 %src3, true, !dbg !24
  call void @llvm.dbg.value(metadata i1 %3, metadata !15, metadata !DIExpression()), !dbg !24
  %4 = xor i1 %src4, true, !dbg !25
  call void @llvm.dbg.value(metadata i1 %4, metadata !16, metadata !DIExpression()), !dbg !25
  %or.v = or i1 %1, %2, !dbg !26
  call void @llvm.dbg.value(metadata i1 %or.v, metadata !17, metadata !DIExpression()), !dbg !26
  %and.v = and i1 %3, %4, !dbg !27
  call void @llvm.dbg.value(metadata i1 %and.v, metadata !18, metadata !DIExpression()), !dbg !27
  br i1 %or.v, label %endif.or, label %then.or, !dbg !28

then.or:                                          ; preds = %entry
  %select.or = select i1 %or.v, float %src1, float %src2, !dbg !29
  call void @llvm.dbg.value(metadata float %select.or, metadata !19, metadata !DIExpression()), !dbg !29
  ret void, !dbg !30

endif.or:                                         ; preds = %entry
  br i1 %and.v, label %endif.and, label %then.and, !dbg !31

then.and:                                         ; preds = %endif.or
  %select.and = select i1 %and.v, float %src1, float %src2, !dbg !32
  call void @llvm.dbg.value(metadata float %select.and, metadata !20, metadata !DIExpression()), !dbg !32
  ret void, !dbg !33

endif.and:                                        ; preds = %endif.or
  ret void, !dbg !34
}

; Testcase 1 MD:
; CHECK-DAG: [[FREM_LOC]] = !DILocation(line: 1
; CHECK-DAG: [[FREM_MD]] = !DILocalVariable(name: "1"
; Testcase 2 MD:
; CHECK-DAG: [[OR_MD]] = !DILocalVariable(name: "6"
; CHECK-DAG: [[OR_LOC]] = !DILocation(line: 6
; CHECK-DAG: [[BRANCH_OR_LOC]] = !DILocation(line: 8
; CHECK-DAG: [[SELECT_OR_LOC]] = !DILocation(line: 9
; Testcase 3 MD:
; CHECK-DAG: [[AND_MD]] = !DILocalVariable(name: "7"
; CHECK-DAG: [[AND_LOC]] = !DILocation(line: 7
; CHECK-DAG: [[BRANCH_AND_LOC]] = !DILocation(line: 11
; CHECK-DAG: [[SELECT_AND_LOC]] = !DILocation(line: 12

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!2}
!llvm.debugify = !{!4, !5}
!llvm.module.flags = !{!6}

!0 = !{void (float, float, i1, i1)* @test_binary, !1}
!1 = !{}
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !1)
!3 = !DIFile(filename: "BinaryOperators.ll", directory: "/")
!4 = !{i32 14}
!5 = !{i32 9}
!6 = !{i32 2, !"Debug Info Version", i32 3}
!7 = distinct !DISubprogram(name: "test_binary", linkageName: "test_binary", scope: null, file: !3, line: 1, type: !8, scopeLine: 1, unit: !2, retainedNodes: !9)
!8 = !DISubroutineType(types: !1)
!9 = !{!10, !12, !14, !15, !16, !17, !18, !19, !20}
!10 = !DILocalVariable(name: "1", scope: !7, file: !3, line: 1, type: !11)
!11 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!12 = !DILocalVariable(name: "2", scope: !7, file: !3, line: 2, type: !13)
!13 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "3", scope: !7, file: !3, line: 3, type: !13)
!15 = !DILocalVariable(name: "4", scope: !7, file: !3, line: 4, type: !13)
!16 = !DILocalVariable(name: "5", scope: !7, file: !3, line: 5, type: !13)
!17 = !DILocalVariable(name: "6", scope: !7, file: !3, line: 6, type: !13)
!18 = !DILocalVariable(name: "7", scope: !7, file: !3, line: 7, type: !13)
!19 = !DILocalVariable(name: "8", scope: !7, file: !3, line: 9, type: !11)
!20 = !DILocalVariable(name: "9", scope: !7, file: !3, line: 12, type: !11)
!21 = !DILocation(line: 1, column: 1, scope: !7)
!22 = !DILocation(line: 2, column: 1, scope: !7)
!23 = !DILocation(line: 3, column: 1, scope: !7)
!24 = !DILocation(line: 4, column: 1, scope: !7)
!25 = !DILocation(line: 5, column: 1, scope: !7)
!26 = !DILocation(line: 6, column: 1, scope: !7)
!27 = !DILocation(line: 7, column: 1, scope: !7)
!28 = !DILocation(line: 8, column: 1, scope: !7)
!29 = !DILocation(line: 9, column: 1, scope: !7)
!30 = !DILocation(line: 10, column: 1, scope: !7)
!31 = !DILocation(line: 11, column: 1, scope: !7)
!32 = !DILocation(line: 12, column: 1, scope: !7)
!33 = !DILocation(line: 13, column: 1, scope: !7)
!34 = !DILocation(line: 14, column: 1, scope: !7)
