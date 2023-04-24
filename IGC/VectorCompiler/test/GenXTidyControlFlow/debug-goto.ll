;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXTidyControlFlow -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXTidyControlFlow
; ------------------------------------------------
; This test checks that GenXTidyControlFlow pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: define dllexport void @test_kernel{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata { <32 x i1>, <32 x i1>, i1 } [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i32> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i32> [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i1> [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i1> [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: [[VAL7_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: void @llvm.dbg.value(metadata <32 x i32> [[VAL3_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK: [[VAL9_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL9_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i32> [[VAL9_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC]]
; CHECK: [[VAL10_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL10_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata { <32 x i1>, i1 } [[VAL10_V]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC]]

define dllexport void @test_kernel(<32 x i32>* %a, <32 x i1> %b) #1 !dbg !8 {
entry:
  %0 = load <32 x i32>, <32 x i32>* %a, !dbg !26
  call void @llvm.dbg.value(metadata <32 x i32> %0, metadata !11, metadata !DIExpression()), !dbg !26
  br label %lbl, !dbg !27

lbl:                                              ; preds = %lbl1, %entry
  %1 = call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer, <32 x i1> %b), !dbg !28
  call void @llvm.dbg.value(metadata { <32 x i1>, <32 x i1>, i1 } %1, metadata !13, metadata !DIExpression()), !dbg !28
  %2 = add <32 x i32> %0, %0, !dbg !29
  call void @llvm.dbg.value(metadata <32 x i32> %2, metadata !15, metadata !DIExpression()), !dbg !29
  %3 = and <32 x i32> %2, %0, !dbg !30
  call void @llvm.dbg.value(metadata <32 x i32> %3, metadata !16, metadata !DIExpression()), !dbg !30
  %4 = extractvalue { <32 x i1>, <32 x i1>, i1 } %1, 0, !dbg !31
  call void @llvm.dbg.value(metadata <32 x i1> %4, metadata !17, metadata !DIExpression()), !dbg !31
  %5 = extractvalue { <32 x i1>, <32 x i1>, i1 } %1, 1, !dbg !32
  call void @llvm.dbg.value(metadata <32 x i1> %5, metadata !19, metadata !DIExpression()), !dbg !32
  %6 = extractvalue { <32 x i1>, <32 x i1>, i1 } %1, 2, !dbg !33
  call void @llvm.dbg.value(metadata i1 %6, metadata !20, metadata !DIExpression()), !dbg !33
  br i1 %6, label %exit, label %lbl1, !dbg !34

lbl1:                                             ; preds = %lbl
  call void @llvm.dbg.value(metadata <32 x i32> %2, metadata !22, metadata !DIExpression()), !dbg !35
  br label %lbl, !dbg !36

exit:                                             ; preds = %lbl
  %7 = xor <32 x i32> %2, %3, !dbg !37
  call void @llvm.dbg.value(metadata <32 x i32> %7, metadata !23, metadata !DIExpression()), !dbg !37
  store <32 x i32> %7, <32 x i32>* %a, !dbg !38
  %8 = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %4, <32 x i1> %5), !dbg !39
  call void @llvm.dbg.value(metadata { <32 x i1>, i1 } %8, metadata !24, metadata !DIExpression()), !dbg !39
  ret void, !dbg !40
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "goto.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
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
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])


declare { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>)

declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }
attributes #1 = { "CMGenxMain" }

!llvm.debugify = !{!0, !1, !2, !3}
!llvm.module.flags = !{!4}
!llvm.dbg.cu = !{!5}

!0 = !{i32 13}
!1 = !{i32 9}
!2 = !{i32 15}
!3 = !{i32 10}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = distinct !DICompileUnit(language: DW_LANG_C, file: !6, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !7)
!6 = !DIFile(filename: "goto.ll", directory: "/")
!7 = !{}
!8 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !6, line: 1, type: !9, scopeLine: 1, unit: !5, retainedNodes: !10)
!9 = !DISubroutineType(types: !7)
!10 = !{!11, !13, !15, !16, !17, !19, !20, !22, !23, !24}
!11 = !DILocalVariable(name: "1", scope: !8, file: !6, line: 1, type: !12)
!12 = !DIBasicType(name: "ty1024", size: 1024, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "2", scope: !8, file: !6, line: 3, type: !14)
!14 = !DIBasicType(name: "ty768", size: 768, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "3", scope: !8, file: !6, line: 4, type: !12)
!16 = !DILocalVariable(name: "4", scope: !8, file: !6, line: 5, type: !12)
!17 = !DILocalVariable(name: "5", scope: !8, file: !6, line: 6, type: !18)
!18 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!19 = !DILocalVariable(name: "6", scope: !8, file: !6, line: 7, type: !18)
!20 = !DILocalVariable(name: "7", scope: !8, file: !6, line: 8, type: !21)
!21 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!22 = !DILocalVariable(name: "8", scope: !8, file: !6, line: 10, type: !12)
!23 = !DILocalVariable(name: "9", scope: !8, file: !6, line: 12, type: !12)
!24 = !DILocalVariable(name: "10", scope: !8, file: !6, line: 14, type: !25)
!25 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!26 = !DILocation(line: 1, column: 1, scope: !8)
!27 = !DILocation(line: 2, column: 1, scope: !8)
!28 = !DILocation(line: 3, column: 1, scope: !8)
!29 = !DILocation(line: 4, column: 1, scope: !8)
!30 = !DILocation(line: 5, column: 1, scope: !8)
!31 = !DILocation(line: 6, column: 1, scope: !8)
!32 = !DILocation(line: 7, column: 1, scope: !8)
!33 = !DILocation(line: 8, column: 1, scope: !8)
!34 = !DILocation(line: 9, column: 1, scope: !8)
!35 = !DILocation(line: 10, column: 1, scope: !8)
!36 = !DILocation(line: 11, column: 1, scope: !8)
!37 = !DILocation(line: 12, column: 1, scope: !8)
!38 = !DILocation(line: 13, column: 1, scope: !8)
!39 = !DILocation(line: 14, column: 1, scope: !8)
!40 = !DILocation(line: 15, column: 1, scope: !8)
