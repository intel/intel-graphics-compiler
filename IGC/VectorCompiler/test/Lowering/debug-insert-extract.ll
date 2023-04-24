;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXLowering
; ------------------------------------------------
; This test checks that GenXLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; CHECK: void @test_lowering{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <1 x i32> [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]

define void @test_lowering(<4 x i32> %a, <4 x i32>* %b, i32* %c) !dbg !6 {
entry:
  %0 = insertelement <4 x i32> %a, i32 13, i32 1, !dbg !16
  call void @llvm.dbg.value(metadata <4 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !16
  %1 = extractelement <4 x i32> %a, i32 0, !dbg !17
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !17
  %2 = add i32 %1, 16, !dbg !18
  call void @llvm.dbg.value(metadata i32 %2, metadata !13, metadata !DIExpression()), !dbg !18
  %3 = insertelement <1 x i32> undef, i32 %2, i32 0, !dbg !19
  call void @llvm.dbg.value(metadata <1 x i32> %3, metadata !14, metadata !DIExpression()), !dbg !19
  %4 = extractelement <1 x i32> %3, i32 0, !dbg !20
  call void @llvm.dbg.value(metadata i32 %4, metadata !15, metadata !DIExpression()), !dbg !20
  store i32 %4, i32* %c, !dbg !21
  store <4 x i32> %0, <4 x i32>* %b, !dbg !22
  ret void, !dbg !23
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "insert-extract.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_lowering", linkageName: "test_lowering", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "insert-extract.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_lowering", linkageName: "test_lowering", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
!23 = !DILocation(line: 8, column: 1, scope: !6)
