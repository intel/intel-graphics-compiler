;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-error-check -S < %s | FileCheck %s
; ------------------------------------------------
; ErrorCheck
; ------------------------------------------------
; This test checks that ErrorCheck pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; This is essentially analisys pass, check that nothing is modified.
; ------------------------------------------------

; CHECK: @test_error{{.*}} !dbg [[SCOPE:![0-9]*]]
;
; CHECK: entry:
; CHECK: [[LOAD1_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK: [[LOAD2_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD2_V]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK: [[DP_V:%[A-z0-9]*]] = call i32{{.*}} !dbg [[DP_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[DP_V]], metadata [[DP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DP_LOC]]
; CHECK: store i32{{.*}} !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[FPTOI_V:%[A-z0-9]*]] = fptoui{{.*}} !dbg [[FPTOI_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <32 x i64> [[FPTOI_V]], metadata [[FPTOI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FPTOI_LOC]]
; CHECK: [[ADD_V:%[A-z0-9]*]] = add{{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <32 x i64> [[ADD_V]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: store <32 x i64>{{.*}} !dbg [[STORE2_LOC:![0-9]*]]

define void @test_error(i32* %src, i32* %dst, <32 x double> %src3, <32 x i64> %src4, <32 x i64>* %dst2, <256 x i64> %src5) !dbg !16 {
entry:
  %0 = load i32, i32* %src, !dbg !26
  call void @llvm.dbg.value(metadata i32 %0, metadata !19, metadata !DIExpression()), !dbg !26
  %1 = load i32, i32* %dst, !dbg !27
  call void @llvm.dbg.value(metadata i32 %1, metadata !21, metadata !DIExpression()), !dbg !27
  %2 = call i32 @llvm.genx.GenISA.dp4a.ss(i32 4, i32 %0, i32 %1), !dbg !28
  call void @llvm.dbg.value(metadata i32 %2, metadata !22, metadata !DIExpression()), !dbg !28
  store i32 %2, i32* %dst, !dbg !29
  %3 = fptoui <32 x double> %src3 to <32 x i64>, !dbg !30
  call void @llvm.dbg.value(metadata <32 x i64> %3, metadata !23, metadata !DIExpression()), !dbg !30
  %4 = add <32 x i64> %3, %src4, !dbg !31
  call void @llvm.dbg.value(metadata <32 x i64> %4, metadata !25, metadata !DIExpression()), !dbg !31
  store <32 x i64> %4, <32 x i64>* %dst2, !dbg !32
  ret void, !dbg !33
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ErrorCheck.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_error", linkageName: "test_error", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[DP_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[DP_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FPTOI_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[FPTOI_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

declare i32 @llvm.genx.GenISA.dp4a.ss(i32, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!IGCMetadata = !{!4}
!llvm.dbg.cu = !{!10}
!llvm.debugify = !{!13, !14}
!llvm.module.flags = !{!15}

!0 = !{void (i32*, i32*, <32 x double>, <32 x i64>, <32 x i64>*, <256 x i64>)* @test_error, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (i32*, i32*, <32 x double>, <32 x i64>, <32 x i64>*, <256 x i64>)* @test_error}
!7 = !{!"FuncMDValue[0]", !8, !9}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
!10 = distinct !DICompileUnit(language: DW_LANG_C, file: !11, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !12)
!11 = !DIFile(filename: "ErrorCheck.ll", directory: "/")
!12 = !{}
!13 = !{i32 8}
!14 = !{i32 5}
!15 = !{i32 2, !"Debug Info Version", i32 3}
!16 = distinct !DISubprogram(name: "test_error", linkageName: "test_error", scope: null, file: !11, line: 1, type: !17, scopeLine: 1, unit: !10, retainedNodes: !18)
!17 = !DISubroutineType(types: !12)
!18 = !{!19, !21, !22, !23, !25}
!19 = !DILocalVariable(name: "1", scope: !16, file: !11, line: 1, type: !20)
!20 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "2", scope: !16, file: !11, line: 2, type: !20)
!22 = !DILocalVariable(name: "3", scope: !16, file: !11, line: 3, type: !20)
!23 = !DILocalVariable(name: "4", scope: !16, file: !11, line: 5, type: !24)
!24 = !DIBasicType(name: "ty2048", size: 2048, encoding: DW_ATE_unsigned)
!25 = !DILocalVariable(name: "5", scope: !16, file: !11, line: 6, type: !24)
!26 = !DILocation(line: 1, column: 1, scope: !16)
!27 = !DILocation(line: 2, column: 1, scope: !16)
!28 = !DILocation(line: 3, column: 1, scope: !16)
!29 = !DILocation(line: 4, column: 1, scope: !16)
!30 = !DILocation(line: 5, column: 1, scope: !16)
!31 = !DILocation(line: 6, column: 1, scope: !16)
!32 = !DILocation(line: 7, column: 1, scope: !16)
!33 = !DILocation(line: 8, column: 1, scope: !16)
