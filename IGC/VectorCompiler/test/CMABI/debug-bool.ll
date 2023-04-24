;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; CMABI
; ------------------------------------------------
; This test checks that CMABI pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: define dllexport void @test_cmabi{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: void @llvm.dbg.value(metadata <2 x i1> [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <2 x i1>* [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <2 x i1> [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]

define dllexport void @test_cmabi(<2 x i1> %a, i1 %b) !dbg !14 {
entry:
  call void @llvm.dbg.value(metadata <2 x i1> %a, metadata !17, metadata !DIExpression()), !dbg !24
  call void @llvm.dbg.value(metadata i1 %b, metadata !19, metadata !DIExpression()), !dbg !25
  %0 = alloca <2 x i1>, !dbg !26
  call void @llvm.dbg.value(metadata <2 x i1>* %0, metadata !21, metadata !DIExpression()), !dbg !26
  %1 = insertelement <2 x i1> %a, i1 %b, i32 0, !dbg !27
  call void @llvm.dbg.value(metadata <2 x i1> %1, metadata !23, metadata !DIExpression()), !dbg !27
  store <2 x i1> %1, <2 x i1>* %0, !dbg !28
  ret void, !dbg !29
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "bool.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_cmabi", linkageName: "test_cmabi", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!4}
!llvm.dbg.cu = !{!9}
!llvm.debugify = !{!11, !12}
!llvm.module.flags = !{!13}

!0 = !{void (<2 x i1>, i1)* @test_cmabi, !"test_cmabi", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 0, i32 0}
!2 = !{i32 0}
!3 = !{}
!4 = !{void (<2 x i1>, i1)* @test_cmabi, null, null, !5, null}
!5 = !{!6}
!6 = !{i32 0, !7}
!7 = !{!8}
!8 = !{i32 2, i32 0}
!9 = distinct !DICompileUnit(language: DW_LANG_C, file: !10, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !3)
!10 = !DIFile(filename: "bool.ll", directory: "/")
!11 = !{i32 6}
!12 = !{i32 4}
!13 = !{i32 2, !"Debug Info Version", i32 3}
!14 = distinct !DISubprogram(name: "test_cmabi", linkageName: "test_cmabi", scope: null, file: !10, line: 1, type: !15, scopeLine: 1, unit: !9, retainedNodes: !16)
!15 = !DISubroutineType(types: !3)
!16 = !{!17, !19, !21, !23}
!17 = !DILocalVariable(name: "1", scope: !14, file: !10, line: 1, type: !18)
!18 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!19 = !DILocalVariable(name: "2", scope: !14, file: !10, line: 2, type: !20)
!20 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "3", scope: !14, file: !10, line: 3, type: !22)
!22 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!23 = !DILocalVariable(name: "4", scope: !14, file: !10, line: 4, type: !18)
!24 = !DILocation(line: 1, column: 1, scope: !14)
!25 = !DILocation(line: 2, column: 1, scope: !14)
!26 = !DILocation(line: 3, column: 1, scope: !14)
!27 = !DILocation(line: 4, column: 1, scope: !14)
!28 = !DILocation(line: 5, column: 1, scope: !14)
!29 = !DILocation(line: 6, column: 1, scope: !14)
