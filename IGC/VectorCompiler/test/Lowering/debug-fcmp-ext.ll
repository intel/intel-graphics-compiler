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
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL1_V:%[A-z0-9.]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL2_V:%[A-z0-9.]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL3_V:%[A-z0-9.]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL4_V:%[A-z0-9.]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL5_V:%[A-z0-9.]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL6_V:%[A-z0-9.]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL7_V:%[A-z0-9.]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL8_V:%[A-z0-9.]*]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V]] = {{.*}}, !dbg [[VAL8_LOC]]

%struct.st = type { float, i1 }

define void @test_lowering(float %a, float* %b, %struct.st %c) !dbg !6 {
entry:
  %0 = extractvalue %struct.st %c, 0, !dbg !21
  call void @llvm.dbg.value(metadata float %0, metadata !9, metadata !DIExpression()), !dbg !21
  %1 = fcmp ord float %a, %0, !dbg !22
  call void @llvm.dbg.value(metadata i1 %1, metadata !11, metadata !DIExpression()), !dbg !22
  %2 = fcmp uno float %a, %0, !dbg !23
  call void @llvm.dbg.value(metadata i1 %2, metadata !13, metadata !DIExpression()), !dbg !23
  %3 = fcmp ueq float %0, %a, !dbg !24
  call void @llvm.dbg.value(metadata i1 %3, metadata !14, metadata !DIExpression()), !dbg !24
  %4 = fcmp one float %0, %a, !dbg !25
  call void @llvm.dbg.value(metadata i1 %4, metadata !15, metadata !DIExpression()), !dbg !25
  %5 = and i1 %1, %2, !dbg !26
  call void @llvm.dbg.value(metadata i1 %5, metadata !16, metadata !DIExpression()), !dbg !26
  %6 = and i1 %3, %4, !dbg !27
  call void @llvm.dbg.value(metadata i1 %6, metadata !17, metadata !DIExpression()), !dbg !27
  %7 = and i1 %5, %6, !dbg !28
  call void @llvm.dbg.value(metadata i1 %7, metadata !18, metadata !DIExpression()), !dbg !28
  %8 = insertvalue %struct.st %c, i1 %7, 1, !dbg !29
  call void @llvm.dbg.value(metadata %struct.st %8, metadata !19, metadata !DIExpression()), !dbg !29
  ret void, !dbg !30
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "fcmp-ext-ins-value.ll", directory: "/")
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
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "fcmp-ext-ins-value.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_lowering", linkageName: "test_lowering", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16, !17, !18, !19}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !12)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !12)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !12)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !20)
!20 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
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
