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
; GenSpecificPattern: And patterns
; ------------------------------------------------
; This test checks that GenSpecificPattern pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------


; ModuleID = 'BinaryOperatorAnd.ll'
source_filename = "BinaryOperatorAnd.ll"

define spir_kernel void @test_binary(double %src1) !dbg !6 {
entry:
; Testcase1:
; CHECK-DAG: [[LINE5:![0-9]+]] = !DILocation(line: 5
; CHECK-DAG: [[AND32_MD:![0-9]+]] = !DILocalVariable(name: "5"
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] i32 [[AND32_V:%[0-9]*]],  metadata [[AND32_MD]]
; CHECK-DAG: [[AND32_V]] = and i32 {{.*}}, 2147483647, !dbg [[LINE5]]
  %0 = fsub double -0.000000e+00, %src1, !dbg !18
  call void @llvm.dbg.value(metadata double %0, metadata !9, metadata !DIExpression()), !dbg !18
  %1 = bitcast double %0 to i64, !dbg !19
  call void @llvm.dbg.value(metadata i64 %1, metadata !11, metadata !DIExpression()), !dbg !19
  %2 = bitcast i64 %1 to <2 x i32>, !dbg !20
  call void @llvm.dbg.value(metadata <2 x i32> %2, metadata !12, metadata !DIExpression()), !dbg !20
  %3 = extractelement <2 x i32> %2, i32 1, !dbg !21
  call void @llvm.dbg.value(metadata i32 %3, metadata !13, metadata !DIExpression()), !dbg !21
  %4 = and i32 %3, 2147483647, !dbg !22
  call void @llvm.dbg.value(metadata i32 %4, metadata !15, metadata !DIExpression()), !dbg !22

; Testcase2:
; CHECK-DAG: [[LINE6:![0-9]+]] = !DILocation(line: 6
; CHECK-DAG: [[AND64_MD:![0-9]+]] = !DILocalVariable(name: "6"
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] i64 [[AND64_V:%[0-9]*]],  metadata [[AND64_MD]]
; CHECK-DAG: [[AND64_V]] = and i64 {{.*}}, 9223372032559808512, !dbg [[LINE6]]
  %5 = and i64 %1, 9223372032559808512, !dbg !23
  call void @llvm.dbg.value(metadata i64 %5, metadata !16, metadata !DIExpression()), !dbg !23

; Testcase3:
; CHECK-DAG: [[LINE7:![0-9]+]] = !DILocation(line: 7
; CHECK-DAG: [[AND64_MD:![0-9]+]] = !DILocalVariable(name: "7"
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] i64 [[AND64_V:%[0-9]*]],  metadata [[AND64_MD]]
; CHECK-DAG: [[AND64_V]] = bitcast <2 x i32> {{.*}}, !dbg [[LINE7]]
  %6 = and i64 %1, -4294967296, !dbg !24
  call void @llvm.dbg.value(metadata i64 %6, metadata !17, metadata !DIExpression()), !dbg !24
  ret void, !dbg !25
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable  }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "BinaryOperatorAnd.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 7}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_binary", linkageName: "test_binary", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !15, !16, !17}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !14)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!18 = !DILocation(line: 1, column: 1, scope: !6)
!19 = !DILocation(line: 2, column: 1, scope: !6)
!20 = !DILocation(line: 3, column: 1, scope: !6)
!21 = !DILocation(line: 4, column: 1, scope: !6)
!22 = !DILocation(line: 5, column: 1, scope: !6)
!23 = !DILocation(line: 6, column: 1, scope: !6)
!24 = !DILocation(line: 7, column: 1, scope: !6)
!25 = !DILocation(line: 8, column: 1, scope: !6)
