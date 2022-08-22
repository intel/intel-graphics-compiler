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
; GenSpecificPattern: Or patterns
; ------------------------------------------------
; This test checks that GenSpecificPattern pass follows
; 'How to Update Debug Info' llvm guideline.
;
; ------------------------------------------------

define spir_kernel void @test_binary(i64 %src1, i64 %src2) !dbg !6 {
entry:
; Testcase1:
; CHECK-DAG: [[LINE3:![0-9]+]] = !DILocation(line: 3
; CHECK-DAG: [[OR_MD:![0-9]+]] = !DILocalVariable(name: "3"
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] i64 [[OR_V:%[0-9]*]],  metadata [[OR_MD]]
; CHECK-DAG: [[OR_V]] = bitcast <2 x i32> {{.*}}, !dbg [[LINE3]]
  %0 = and i64 %src1, 4294967295, !dbg !21
  call void @llvm.dbg.value(metadata i64 %0, metadata !9, metadata !DIExpression()), !dbg !21
  %1 = shl i64 %src2, 32, !dbg !22
  call void @llvm.dbg.value(metadata i64 %1, metadata !11, metadata !DIExpression()), !dbg !22
  %2 = or i64 %0, %1, !dbg !23
  call void @llvm.dbg.value(metadata i64 %2, metadata !12, metadata !DIExpression()), !dbg !23

; Testcase2:
; CHECK-DAG: [[LINE8:![0-9]+]] = !DILocation(line: 8
; CHECK-DAG: [[OR_MD:![0-9]+]] = !DILocalVariable(name: "8"
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] i64 [[OR_V:%[0-9]*]],  metadata [[OR_MD]]
; CHECK-DAG: [[OR_V]] = bitcast <2 x i32> {{.*}}, !dbg [[LINE8]]
  %3 = trunc i64 %src2 to i32, !dbg !24
  call void @llvm.dbg.value(metadata i32 %3, metadata !13, metadata !DIExpression()), !dbg !24
  %4 = and i64 %src1, 4294967295, !dbg !25
  call void @llvm.dbg.value(metadata i64 %4, metadata !15, metadata !DIExpression()), !dbg !25
  %5 = insertelement <2 x i32> <i32 0, i32 undef>, i32 %3, i32 1, !dbg !26
  call void @llvm.dbg.value(metadata <2 x i32> %5, metadata !16, metadata !DIExpression()), !dbg !26
  %6 = bitcast <2 x i32> %5 to i64, !dbg !27
  call void @llvm.dbg.value(metadata i64 %6, metadata !17, metadata !DIExpression()), !dbg !27
  %7 = or i64 %4, %6, !dbg !28

; Testcase3:
; CHECK-DAG: [[LINE10:![0-9]+]] = !DILocation(line: 10
; CHECK-DAG: [[OR_MD:![0-9]+]] = !DILocalVariable(name: "10"
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] i64 [[OR_V:%[0-9]*]],  metadata [[OR_MD]]
; CHECK-DAG: [[OR_V]] = add i64 {{.*}} !dbg [[LINE10]]
  call void @llvm.dbg.value(metadata i64 %7, metadata !18, metadata !DIExpression()), !dbg !28
  %8 = shl i64 %src1, 3, !dbg !29
  call void @llvm.dbg.value(metadata i64 %8, metadata !19, metadata !DIExpression()), !dbg !29
  %9 = or i64 %8, 7, !dbg !30
  call void @llvm.dbg.value(metadata i64 %9, metadata !20, metadata !DIExpression()), !dbg !30
  ret void, !dbg !31
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "BinaryOperatorOr.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 10}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_binary", linkageName: "test_binary", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !15, !16, !17, !18, !19, !20}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !10)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !10)
!20 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !10)
!21 = !DILocation(line: 1, column: 1, scope: !6)
!22 = !DILocation(line: 2, column: 1, scope: !6)
!23 = !DILocation(line: 3, column: 1, scope: !6)
!24 = !DILocation(line: 4, column: 1, scope: !6)
!25 = !DILocation(line: 5, column: 1, scope: !6)
!26 = !DILocation(line: 6, column: 1, scope: !6)
!27 = !DILocation(line: 7, column: 1, scope: !6)
!28 = !DILocation(line: 8, column: 1, scope: !6)
!29 = !DILocation(line: 9, column: 1, scope: !6)
!30 = !DILocation(line: 10, column: 1, scope: !6)
!31 = !DILocation(line: 11, column: 1, scope: !6)
