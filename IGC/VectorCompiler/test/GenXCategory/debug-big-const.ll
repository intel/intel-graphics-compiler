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

; CHECK: define dllexport void @test_categ{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata <64 x i32> [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <64 x i32> [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE_LOC:![0-9]*]]

define dllexport void @test_categ(<64 x i32>* %src, <64 x i32>* %dst) #0 !dbg !17 {
entry:
  %0 = load <64 x i32>, <64 x i32>* %src, !dbg !23
  call void @llvm.dbg.value(metadata <64 x i32> %0, metadata !20, metadata !DIExpression()), !dbg !23
  %1 = add <64 x i32> %0, <i32 1, i32 4, i32 6, i32 5, i32 3, i32 1, i32 2, i32 3, i32 4, i32 31, i32 16, i32 12, i32 65, i32 12, i32 62, i32 55, i32 1, i32 41, i32 26, i32 1111, i32 2, i32 42, i32 36, i32 2211, i32 3, i32 43, i32 46, i32 3311, i32 4, i32 44, i32 56, i32 4141, i32 5, i32 45, i32 76, i32 5511, i32 6, i32 46, i32 86, i32 7711, i32 7, i32 47, i32 96, i32 8811, i32 8, i32 48, i32 46, i32 9911, i32 9, i32 49, i32 166, i32 2311, i32 0, i32 50, i32 26, i32 4211, i32 1, i32 51, i32 16, i32 6511, i32 2, i32 53, i32 326, i32 3711>, !dbg !24
  call void @llvm.dbg.value(metadata <64 x i32> %1, metadata !22, metadata !DIExpression()), !dbg !24
  store <64 x i32> %1, <64 x i32>* %dst, !dbg !25
  ret void, !dbg !26
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "big-const.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_categ", linkageName: "test_categ", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}
!llvm.dbg.cu = !{!12}
!llvm.debugify = !{!14, !15}
!llvm.module.flags = !{!16}

!0 = !{void (<64 x i32>*, <64 x i32>*)* @test_categ, !"test_categ", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 112, i32 0, i32 96}
!2 = !{i32 -1, i32 144, i32 64}
!3 = !{i32 0, i32 0}
!4 = !{}
!5 = !{void (<64 x i32>*, <64 x i32>*)* @test_categ, !6, !7, !8, null}
!6 = !{i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 0}
!8 = !{!9}
!9 = !{i32 0, !10}
!10 = !{!11}
!11 = !{i32 1, i32 0}
!12 = distinct !DICompileUnit(language: DW_LANG_C, file: !13, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!13 = !DIFile(filename: "big-const.ll", directory: "/")
!14 = !{i32 4}
!15 = !{i32 2}
!16 = !{i32 2, !"Debug Info Version", i32 3}
!17 = distinct !DISubprogram(name: "test_categ", linkageName: "test_categ", scope: null, file: !13, line: 1, type: !18, scopeLine: 1, unit: !12, retainedNodes: !19)
!18 = !DISubroutineType(types: !4)
!19 = !{!20, !22}
!20 = !DILocalVariable(name: "1", scope: !17, file: !13, line: 1, type: !21)
!21 = !DIBasicType(name: "ty2048", size: 2048, encoding: DW_ATE_unsigned)
!22 = !DILocalVariable(name: "2", scope: !17, file: !13, line: 2, type: !21)
!23 = !DILocation(line: 1, column: 1, scope: !17)
!24 = !DILocation(line: 2, column: 1, scope: !17)
!25 = !DILocation(line: 3, column: 1, scope: !17)
!26 = !DILocation(line: 4, column: 1, scope: !17)
