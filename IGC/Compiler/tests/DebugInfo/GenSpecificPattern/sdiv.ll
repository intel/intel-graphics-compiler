;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-gen-specific-pattern -S < %s | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: sdiv patterns
; ------------------------------------------------
; This test checks that GenSpecificPattern pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; ModuleID = 'SDivPattern.ll'
source_filename = "SDivPattern.ll"

define spir_kernel void @test_sdiv(i64 %src1) !dbg !6 {
entry:
; Testcase1:
; CHECK-DAG: [[LINE1:![0-9]+]] = !DILocation(line: 1
; CHECK-DAG: [[DIV_MD:![0-9]+]] = !DILocalVariable(name: "1"
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] i64 [[DIV_V:%[0-9]*]],  metadata [[DIV_MD]]
; CHECK-DAG: [[DIV_V]] = ashr i64 {{.*}}, 1, !dbg [[LINE1]]
  %0 = sdiv i64 %src1, 2, !dbg !15
  call void @llvm.dbg.value(metadata i64 %0, metadata !9, metadata !DIExpression()), !dbg !15

; Testcase2:
; CHECK-DAG: [[LINE3:![0-9]+]] = !DILocation(line: 3
; CHECK-DAG: [[DIV_MD:![0-9]+]] = !DILocalVariable(name: "3"
; CHECK-DAG: [[DBG_VALUE_CALL]] i32 [[DIV_V:%[0-9]*]],  metadata [[DIV_MD]]
; CHECK-DAG: [[DIV_V]] = ashr i32 {{.*}}, 1, !dbg [[LINE3]]
  %1 = trunc i64 %src1 to i32, !dbg !16
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !16
  %2 = sdiv i32 %1, 2, !dbg !17
  call void @llvm.dbg.value(metadata i32 %2, metadata !13, metadata !DIExpression()), !dbg !17

; Testcase3:
; CHECK-DAG: [[LINE4:![0-9]+]] = !DILocation(line: 4
; CHECK-DAG: [[DIV_MD:![0-9]+]] = !DILocalVariable(name: "4"
; CHECK-DAG: [[DBG_VALUE_CALL]] i32 [[DIV_V:%[0-9]*]],  metadata [[DIV_MD]]
; CHECK-DAG: [[DIV_V]] = ashr i32 {{.*}}, 2, !dbg [[LINE4]]
  %3 = sdiv i32 %2, 4, !dbg !18
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !18
  ret void, !dbg !19
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "SDivPattern.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_sdiv", linkageName: "test_sdiv", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocation(line: 1, column: 1, scope: !6)
!16 = !DILocation(line: 2, column: 1, scope: !6)
!17 = !DILocation(line: 3, column: 1, scope: !6)
!18 = !DILocation(line: 4, column: 1, scope: !6)
!19 = !DILocation(line: 5, column: 1, scope: !6)
