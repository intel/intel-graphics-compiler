;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXLegalization
; ------------------------------------------------
; This test checks that GenXLegalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: void @test_transform{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: call void @test_use(i32 [[VAL1_V:%[A-z0-9.]*]]), !dbg [[CALL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]

; CHECK-DAG: call void @test_use(i32 [[VAL2_V:%[A-z0-9.]*]]), !dbg [[CALL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]

; CHECK-DAG: call void @test_use(i32 [[VAL3_V:%[A-z0-9.]*]]), !dbg [[CALL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]

define void @test_transform(i32* %a) !dbg !6 {
entry:
  %0 = call i32 @llvm.genx.predefined.surface(i32 1), !dbg !13
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !13
  call void @test_use(i32 %0), !dbg !14
  br label %bb1, !dbg !15

bb1:                                              ; preds = %entry
  %1 = call i32 @llvm.genx.predefined.surface(i32 2), !dbg !16
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !16
  call void @test_use(i32 %1), !dbg !17
  br label %bb2, !dbg !18

bb2:                                              ; preds = %bb1
  %2 = call i32 @llvm.genx.predefined.surface(i32 3), !dbg !19
  call void @llvm.dbg.value(metadata i32 %2, metadata !12, metadata !DIExpression()), !dbg !19
  call void @test_use(i32 %2), !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "intrinsic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_transform", linkageName: "test_transform", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

; CHECK-DAG: [[CALL1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL3_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

declare i32 @llvm.genx.predefined.surface(i32)

declare void @test_use(i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "intrinsic.ll", directory: "/")
!2 = !{}
!3 = !{i32 9}
!4 = !{i32 3}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_transform", linkageName: "test_transform", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 4, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 7, type: !10)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
!17 = !DILocation(line: 5, column: 1, scope: !6)
!18 = !DILocation(line: 6, column: 1, scope: !6)
!19 = !DILocation(line: 7, column: 1, scope: !6)
!20 = !DILocation(line: 8, column: 1, scope: !6)
!21 = !DILocation(line: 9, column: 1, scope: !6)
