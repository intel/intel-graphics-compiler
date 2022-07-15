;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-PartialEmuI64Ops -S < %s | FileCheck %s
; ------------------------------------------------
; PartialEmuI64Ops : binary operands
; ------------------------------------------------
; This test checks that PartialEmuI64Ops pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @test_part64{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: = {{.*}}, !dbg [[ADD_LOC:![0-9]*]]
; Value is undef here, check DIExpr before filing
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: = {{.*}}, !dbg [[SUB_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[SUB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SUB_LOC]]
; CHECK: = {{.*}}, !dbg [[MUL_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[MUL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[MUL_LOC]]
; CHECK: = {{.*}}, !dbg [[AND_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[AND_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AND_LOC]]
; CHECK: = {{.*}}, !dbg [[OR_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[OR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[OR_LOC]]
; CHECK: = {{.*}}, !dbg [[XOR_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[XOR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[XOR_LOC]]
; CHECK-DAG: llvm.dbg.value(metadata i1 [[ICMP_V:%[A-z0-9]*]], metadata [[ICMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ICMP_LOC:![0-9]*]]
; CHECK-DAG: [[ICMP_V]] = {{.*}}, !dbg [[ICMP_LOC]]
; CHECK: = {{.*}}, !dbg [[SELECT_LOC:![0-9]*]]
; CHECK: llvm.dbg.value(metadata {{.*}}, metadata [[SELECT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT_LOC]]

define void @test_part64(i64 %a, i64 %b, i64* %dst) !dbg !9 {
  %1 = add i64 %a, %b, !dbg !22
  call void @llvm.dbg.value(metadata i64 %1, metadata !12, metadata !DIExpression()), !dbg !22
  %2 = sub i64 %a, 13, !dbg !23
  call void @llvm.dbg.value(metadata i64 %2, metadata !14, metadata !DIExpression()), !dbg !23
  %3 = mul i64 %1, %2, !dbg !24
  call void @llvm.dbg.value(metadata i64 %3, metadata !15, metadata !DIExpression()), !dbg !24
  %4 = and i64 %3, %1, !dbg !25
  call void @llvm.dbg.value(metadata i64 %4, metadata !16, metadata !DIExpression()), !dbg !25
  %5 = or i64 %4, %a, !dbg !26
  call void @llvm.dbg.value(metadata i64 %5, metadata !17, metadata !DIExpression()), !dbg !26
  %6 = xor i64 %5, %b, !dbg !27
  call void @llvm.dbg.value(metadata i64 %6, metadata !18, metadata !DIExpression()), !dbg !27
  %7 = icmp sge i64 %6, %a, !dbg !28
  call void @llvm.dbg.value(metadata i1 %7, metadata !19, metadata !DIExpression()), !dbg !28
  %8 = select i1 %7, i64 %a, i64 %b, !dbg !29
  call void @llvm.dbg.value(metadata i64 %8, metadata !21, metadata !DIExpression()), !dbg !29
  store i64 %8, i64* %dst, !dbg !30
  ret void, !dbg !31
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_part64", linkageName: "test_part64", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SUB_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[SUB_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[MUL_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[MUL_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[AND_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[AND_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[OR_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[OR_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[XOR_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[XOR_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ICMP_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[ICMP_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[SELECT_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{void (i64, i64, i64*)* @test_part64, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "basic.ll", directory: "/")
!5 = !{}
!6 = !{i32 10}
!7 = !{i32 8}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_part64", linkageName: "test_part64", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !16, !17, !18, !19, !21}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !13)
!16 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !13)
!17 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 5, type: !13)
!18 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 6, type: !13)
!19 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 7, type: !20)
!20 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "8", scope: !9, file: !4, line: 8, type: !13)
!22 = !DILocation(line: 1, column: 1, scope: !9)
!23 = !DILocation(line: 2, column: 1, scope: !9)
!24 = !DILocation(line: 3, column: 1, scope: !9)
!25 = !DILocation(line: 4, column: 1, scope: !9)
!26 = !DILocation(line: 5, column: 1, scope: !9)
!27 = !DILocation(line: 6, column: 1, scope: !9)
!28 = !DILocation(line: 7, column: 1, scope: !9)
!29 = !DILocation(line: 8, column: 1, scope: !9)
!30 = !DILocation(line: 9, column: 1, scope: !9)
!31 = !DILocation(line: 10, column: 1, scope: !9)
