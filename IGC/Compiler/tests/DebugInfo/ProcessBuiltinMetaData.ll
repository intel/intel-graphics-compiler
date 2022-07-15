;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-process-builtin-metaData -S < %s | FileCheck %s
; ------------------------------------------------
; ProcessBuiltinMetaData
; ------------------------------------------------
; This test checks that ProcessBuiltinMetaData pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;
; This pass updates non debug MD, check that debug is unmodified.

; CHECK: @test_processmd{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: call void {{.*}}, !dbg [[CALL_LOC:![0-9]*]]

; CHECK: @user_processmd{{.*}} !dbg [[SCOPE2:![0-9]*]]
;
; CHECK: entry:
; CHECK: [[LOAD1_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK: [[LOAD2_V:%[A-z0-9]*]] = load i32{{.*}} !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[LOAD2_V]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK: [[ADD_V:%[A-z0-9]*]] = add{{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i32 [[ADD_V]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: store i32{{.*}} !dbg [[STORE1_LOC:![0-9]*]]

define void @test_processmd(i32* %src, i32* %dst) !dbg !16 {
  call void @user_processmd(i32* %src, i32* %dst), !dbg !18
  ret void, !dbg !19
}

define void @user_processmd(i32* %src, i32* %dst) !dbg !20 {
entry:
  %0 = load i32, i32* %src, !dbg !26
  call void @llvm.dbg.value(metadata i32 %0, metadata !22, metadata !DIExpression()), !dbg !26
  %1 = load i32, i32* %dst, !dbg !27
  call void @llvm.dbg.value(metadata i32 %1, metadata !24, metadata !DIExpression()), !dbg !27
  %2 = add i32 %0, %1, !dbg !28
  call void @llvm.dbg.value(metadata i32 %2, metadata !25, metadata !DIExpression()), !dbg !28
  store i32 %2, i32* %dst, !dbg !29
  ret void, !dbg !30
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ProcessBuiltinMetaData.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_processmd", linkageName: "test_processmd", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
;
; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "user_processmd", linkageName: "user_processmd", scope: null, file: [[FILE]], line: 3
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE2]], file: [[FILE]], line: 3
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE2]], file: [[FILE]], line: 4
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE2]], file: [[FILE]], line: 5
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE2]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!IGCMetadata = !{!4}
!llvm.dbg.cu = !{!10}
!llvm.debugify = !{!13, !14}
!llvm.module.flags = !{!15}

!0 = !{void (i32*, i32*)* @test_processmd, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (i32*, i32*)* @test_processmd}
!7 = !{!"FuncMDValue[0]", !8, !9}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
!10 = distinct !DICompileUnit(language: DW_LANG_C, file: !11, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !12)
!11 = !DIFile(filename: "ProcessBuiltinMetaData.ll", directory: "/")
!12 = !{}
!13 = !{i32 7}
!14 = !{i32 3}
!15 = !{i32 2, !"Debug Info Version", i32 3}
!16 = distinct !DISubprogram(name: "test_processmd", linkageName: "test_processmd", scope: null, file: !11, line: 1, type: !17, scopeLine: 1, unit: !10, retainedNodes: !12)
!17 = !DISubroutineType(types: !12)
!18 = !DILocation(line: 1, column: 1, scope: !16)
!19 = !DILocation(line: 2, column: 1, scope: !16)
!20 = distinct !DISubprogram(name: "user_processmd", linkageName: "user_processmd", scope: null, file: !11, line: 3, type: !17, scopeLine: 3, unit: !10, retainedNodes: !21)
!21 = !{!22, !24, !25}
!22 = !DILocalVariable(name: "1", scope: !20, file: !11, line: 3, type: !23)
!23 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!24 = !DILocalVariable(name: "2", scope: !20, file: !11, line: 4, type: !23)
!25 = !DILocalVariable(name: "3", scope: !20, file: !11, line: 5, type: !23)
!26 = !DILocation(line: 3, column: 1, scope: !20)
!27 = !DILocation(line: 4, column: 1, scope: !20)
!28 = !DILocation(line: 5, column: 1, scope: !20)
!29 = !DILocation(line: 6, column: 1, scope: !20)
!30 = !DILocation(line: 7, column: 1, scope: !20)
