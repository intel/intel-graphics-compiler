;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;
; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXLegalization
; ------------------------------------------------
; This test checks that GenXLegalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; Current test check correctness of attached debug locations for all generated instructions
; for legalization genx_any and genx_all instructions
;

; CHECK: i1 @test_allany{{.*}} !dbg [[SCOPE1:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i1> [[VAL2_V:%[A-z0-9.]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL3_V:%[A-z0-9.]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}[[VAL3T_V:%[A-z0-9.]*]]{{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: [[VAL3T_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL4_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]

define i1 @test_allany(<32 x i32>* %a) !dbg !6 {
entry:
  %0 = load <32 x i32>, <32 x i32>* %a, !dbg !16
  call void @llvm.dbg.value(metadata <32 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !16
  %1 = icmp eq <32 x i32> zeroinitializer, %0, !dbg !17
  call void @llvm.dbg.value(metadata <32 x i1> %1, metadata !9, metadata !DIExpression()), !dbg !17
  %2 = call i1 @llvm.genx.any.v32i1(<32 x i1> %1), !dbg !18
  call void @llvm.dbg.value(metadata i1 %2, metadata !9, metadata !DIExpression()), !dbg !18
  %3 = xor i1 %2, true, !dbg !19
  call void @llvm.dbg.value(metadata i1 %3, metadata !9, metadata !DIExpression()), !dbg !19
  ret i1 %3, !dbg !20
}

; CHECK: i1 @test_allanysplit{{.*}} !dbg [[SCOPE2:![0-9]*]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <64 x i32> [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value({{.*}}), !dbg [[VAL6_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL7_V:%[A-z0-9.]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}[[VAL7T_V:%[A-z0-9.]*]]{{.*}}, !dbg [[VAL7_LOC]]
; CHECK-DAG: [[VAL7T_V]] = {{.*}}, !dbg [[VAL7_LOC]]

define i1 @test_allanysplit(<64 x i32>* %a) !dbg !21 {
entry:
  %0 = load <64 x i32>, <64 x i32>* %a, !dbg !29
  call void @llvm.dbg.value(metadata <64 x i32> %0, metadata !23, metadata !DIExpression()), !dbg !29
  %1 = icmp eq <64 x i32> zeroinitializer, %0, !dbg !30
  call void @llvm.dbg.value(metadata <64 x i1> %1, metadata !23, metadata !DIExpression()), !dbg !30
  %2 = call i1 @llvm.genx.all.v64i1(<64 x i1> %1), !dbg !31
  call void @llvm.dbg.value(metadata i1 %2, metadata !23, metadata !DIExpression()), !dbg !31
  %3 = xor i1 %2, true, !dbg !32
  call void @llvm.dbg.value(metadata i1 %3, metadata !23, metadata !DIExpression()), !dbg !32
  ret i1 %3, !dbg !33
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "allany.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test_allany", linkageName: "test_allany", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE1]])

; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "test_allanysplit", linkageName: "test_allanysplit", scope: null, file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE2]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE2]])


declare i1 @llvm.genx.any.v32i1(<32 x i1>)

declare i1 @llvm.genx.all.v64i1(<64 x i1>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "allany.ll", directory: "/")
!2 = !{}
!3 = !{i32 10}
!4 = !{i32 8}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_allany", linkageName: "test_allany", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty1024", size: 1024, encoding: DW_ATE_unsigned)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = distinct !DISubprogram(name: "test_allanysplit", linkageName: "test_allanysplit", scope: null, file: !1, line: 6, type: !7, scopeLine: 6, unit: !0, retainedNodes: !22)
!22 = !{!23}
!23 = !DILocalVariable(name: "5", scope: !21, file: !1, line: 6, type: !24)
!24 = !DIBasicType(name: "ty2048", size: 2048, encoding: DW_ATE_unsigned)
!29 = !DILocation(line: 6, column: 1, scope: !21)
!30 = !DILocation(line: 7, column: 1, scope: !21)
!31 = !DILocation(line: 8, column: 1, scope: !21)
!32 = !DILocation(line: 9, column: 1, scope: !21)
!33 = !DILocation(line: 10, column: 1, scope: !21)

