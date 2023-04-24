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
; CHECK-DAG: void @llvm.dbg.value(metadata %struct.st [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK: bb1
; CHECK-DAG: void @llvm.dbg.value(metadata %struct.st [[VAL2_V:%[A-z0-9.]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata %struct.st [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata %struct.st [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata %struct.st [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK: end:
; CHECK: store{{.*}}, !dbg [[STORE_LOC:![0-9]*]]


%struct.st = type { i1, <4 x float>, <2 x i64> }

define dllexport void @test_categ(%struct.st* %src, %struct.st* %dst) #0 !dbg !17 {
entry:
  %0 = load %struct.st, %struct.st* %src, !dbg !28
  call void @llvm.dbg.value(metadata %struct.st %0, metadata !20, metadata !DIExpression()), !dbg !28
  br label %bb1, !dbg !29

bb1:                                              ; preds = %bb1, %entry
  %1 = phi %struct.st [ %0, %entry ], [ %2, %bb1 ], !dbg !30
  call void @llvm.dbg.value(metadata %struct.st %1, metadata !22, metadata !DIExpression()), !dbg !30
  %2 = load %struct.st, %struct.st* %dst, !dbg !31
  call void @llvm.dbg.value(metadata %struct.st %2, metadata !23, metadata !DIExpression()), !dbg !31
  %3 = extractvalue %struct.st %1, 0, !dbg !32
  call void @llvm.dbg.value(metadata i1 %3, metadata !24, metadata !DIExpression()), !dbg !32
  %4 = insertvalue %struct.st %1, <2 x i64> <i64 13, i64 15>, 2, !dbg !33
  call void @llvm.dbg.value(metadata %struct.st %4, metadata !26, metadata !DIExpression()), !dbg !33
  %5 = select i1 %3, %struct.st %2, %struct.st %4, !dbg !34
  call void @llvm.dbg.value(metadata %struct.st %5, metadata !27, metadata !DIExpression()), !dbg !34
  br i1 %3, label %end, label %bb1, !dbg !35

end:                                              ; preds = %bb1
  store %struct.st %5, %struct.st* %dst, !dbg !36
  ret void, !dbg !37
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "phi-struct.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_categ", linkageName: "test_categ", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}
!llvm.dbg.cu = !{!12}
!llvm.debugify = !{!14, !15}
!llvm.module.flags = !{!16}

!0 = !{void (%struct.st*, %struct.st*)* @test_categ, !"test_categ", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 112, i32 0, i32 96}
!2 = !{i32 -1, i32 144, i32 64}
!3 = !{i32 0, i32 0}
!4 = !{}
!5 = !{void (%struct.st*, %struct.st*)* @test_categ, !6, !7, !8, null}
!6 = !{i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 0}
!8 = !{!9}
!9 = !{i32 0, !10}
!10 = !{!11}
!11 = !{i32 1, i32 0}
!12 = distinct !DICompileUnit(language: DW_LANG_C, file: !13, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!13 = !DIFile(filename: "phi-struct.ll", directory: "/")
!14 = !{i32 10}
!15 = !{i32 6}
!16 = !{i32 2, !"Debug Info Version", i32 3}
!17 = distinct !DISubprogram(name: "test_categ", linkageName: "test_categ", scope: null, file: !13, line: 1, type: !18, scopeLine: 1, unit: !12, retainedNodes: !19)
!18 = !DISubroutineType(types: !4)
!19 = !{!20, !22, !23, !24, !26, !27}
!20 = !DILocalVariable(name: "1", scope: !17, file: !13, line: 1, type: !21)
!21 = !DIBasicType(name: "ty384", size: 384, encoding: DW_ATE_unsigned)
!22 = !DILocalVariable(name: "2", scope: !17, file: !13, line: 3, type: !21)
!23 = !DILocalVariable(name: "3", scope: !17, file: !13, line: 4, type: !21)
!24 = !DILocalVariable(name: "4", scope: !17, file: !13, line: 5, type: !25)
!25 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!26 = !DILocalVariable(name: "5", scope: !17, file: !13, line: 6, type: !21)
!27 = !DILocalVariable(name: "6", scope: !17, file: !13, line: 7, type: !21)
!28 = !DILocation(line: 1, column: 1, scope: !17)
!29 = !DILocation(line: 2, column: 1, scope: !17)
!30 = !DILocation(line: 3, column: 1, scope: !17)
!31 = !DILocation(line: 4, column: 1, scope: !17)
!32 = !DILocation(line: 5, column: 1, scope: !17)
!33 = !DILocation(line: 6, column: 1, scope: !17)
!34 = !DILocation(line: 7, column: 1, scope: !17)
!35 = !DILocation(line: 8, column: 1, scope: !17)
!36 = !DILocation(line: 9, column: 1, scope: !17)
!37 = !DILocation(line: 10, column: 1, scope: !17)
