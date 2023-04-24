;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXCategory
; ------------------------------------------------
; This test checks that GenXCategory pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; No reordering is done, check as it is
;
; CHECK: define dllexport void @test_categ{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i1> [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = add {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i1> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE_LOC:![0-9]*]]

define dllexport void @test_categ(i32* %src, i32* %dst) #0 !dbg !17 {
entry:
  %0 = load i32, i32* %src, !dbg !27
  call void @llvm.dbg.value(metadata i32 %0, metadata !20, metadata !DIExpression()), !dbg !27
  %1 = bitcast i32 12345678 to <32 x i1>, !dbg !28
  call void @llvm.dbg.value(metadata <32 x i1> %1, metadata !22, metadata !DIExpression()), !dbg !28
  %2 = add <32 x i1> %1, <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false>, !dbg !29
  call void @llvm.dbg.value(metadata <32 x i1> %2, metadata !24, metadata !DIExpression()), !dbg !29
  %3 = bitcast <32 x i1> %2 to i32, !dbg !30
  call void @llvm.dbg.value(metadata i32 %3, metadata !25, metadata !DIExpression()), !dbg !30
  %4 = add i32 %0, %3, !dbg !31
  call void @llvm.dbg.value(metadata i32 %4, metadata !26, metadata !DIExpression()), !dbg !31
  store i32 %4, i32* %dst, !dbg !32
  ret void, !dbg !33
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "simple-const.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_categ", linkageName: "test_categ", scope: null, file: [[FILE]], line: 1
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
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}
!llvm.dbg.cu = !{!12}
!llvm.debugify = !{!14, !15}
!llvm.module.flags = !{!16}

!0 = !{void (i32*, i32*)* @test_categ, !"test_categ", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 112, i32 0, i32 96}
!2 = !{i32 -1, i32 144, i32 64}
!3 = !{i32 0, i32 0}
!4 = !{}
!5 = !{void (i32*, i32*)* @test_categ, !6, !7, !8, null}
!6 = !{i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 0}
!8 = !{!9}
!9 = !{i32 0, !10}
!10 = !{!11}
!11 = !{i32 1, i32 0}
!12 = distinct !DICompileUnit(language: DW_LANG_C, file: !13, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!13 = !DIFile(filename: "simple-const.ll", directory: "/")
!14 = !{i32 7}
!15 = !{i32 5}
!16 = !{i32 2, !"Debug Info Version", i32 3}
!17 = distinct !DISubprogram(name: "test_categ", linkageName: "test_categ", scope: null, file: !13, line: 1, type: !18, scopeLine: 1, unit: !12, retainedNodes: !19)
!18 = !DISubroutineType(types: !4)
!19 = !{!20, !22, !24, !25, !26}
!20 = !DILocalVariable(name: "1", scope: !17, file: !13, line: 1, type: !21)
!21 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!22 = !DILocalVariable(name: "2", scope: !17, file: !13, line: 2, type: !23)
!23 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!24 = !DILocalVariable(name: "3", scope: !17, file: !13, line: 3, type: !23)
!25 = !DILocalVariable(name: "4", scope: !17, file: !13, line: 4, type: !21)
!26 = !DILocalVariable(name: "5", scope: !17, file: !13, line: 5, type: !21)
!27 = !DILocation(line: 1, column: 1, scope: !17)
!28 = !DILocation(line: 2, column: 1, scope: !17)
!29 = !DILocation(line: 3, column: 1, scope: !17)
!30 = !DILocation(line: 4, column: 1, scope: !17)
!31 = !DILocation(line: 5, column: 1, scope: !17)
!32 = !DILocation(line: 6, column: 1, scope: !17)
!33 = !DILocation(line: 7, column: 1, scope: !17)
