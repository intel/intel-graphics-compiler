;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXEarlySimdCFConformance -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXEarlySimdCFConformance
; ------------------------------------------------
; This test checks that GenXEarlySimdCFConformance pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: define dllexport void @test_kernel{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <32 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: br {{.*}}, !dbg [[BR1_LOC:![0-9]*]]
;
; CHECK: lbl:
; CHECK-DAG: void @llvm.dbg.value(metadata { <32 x i1>, <32 x i1>, i1 } [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i32> [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i32> [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i1> [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i1> [[VAL6_V:%[A-z0-9]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i1 [[VAL7_V:%[A-z0-9]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK: br i1 {{.*}}, !dbg [[BR2_LOC:![0-9]*]]

; CHECK: exit:
; CHECK-DAG: void @llvm.dbg.value(metadata <32 x i32> [[VAL8_V:%[A-z0-9]*]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata { <32 x i1>, i1 } [[VAL9_V:%[A-z0-9]*]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V]] = {{.*}}, !dbg [[VAL8_LOC]]
; CHECK-DAG: [[VAL9_V]] = {{.*}}, !dbg [[VAL9_LOC]]
; CHECK-DAG: store{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]

define dllexport void @test_kernel(<32 x i32>* %a, <32 x i1> %b) !dbg !6 {
entry:
  %0 = load <32 x i32>, <32 x i32>* %a, !dbg !23
  call void @llvm.dbg.value(metadata <32 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !23
  br label %lbl, !dbg !24

lbl:                                              ; preds = %lbl, %entry
  %1 = call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer, <32 x i1> %b), !dbg !25
  call void @llvm.dbg.value(metadata { <32 x i1>, <32 x i1>, i1 } %1, metadata !11, metadata !DIExpression()), !dbg !25
  %2 = add <32 x i32> %0, %0, !dbg !26
  call void @llvm.dbg.value(metadata <32 x i32> %2, metadata !13, metadata !DIExpression()), !dbg !26
  %3 = and <32 x i32> %2, %0, !dbg !27
  call void @llvm.dbg.value(metadata <32 x i32> %3, metadata !14, metadata !DIExpression()), !dbg !27
  %4 = extractvalue { <32 x i1>, <32 x i1>, i1 } %1, 0, !dbg !28
  call void @llvm.dbg.value(metadata <32 x i1> %4, metadata !15, metadata !DIExpression()), !dbg !28
  %5 = extractvalue { <32 x i1>, <32 x i1>, i1 } %1, 1, !dbg !29
  call void @llvm.dbg.value(metadata <32 x i1> %5, metadata !17, metadata !DIExpression()), !dbg !29
  %6 = extractvalue { <32 x i1>, <32 x i1>, i1 } %1, 2, !dbg !30
  call void @llvm.dbg.value(metadata i1 %6, metadata !18, metadata !DIExpression()), !dbg !30
  br i1 %6, label %exit, label %lbl, !dbg !31

exit:                                             ; preds = %lbl
  %7 = xor <32 x i32> %3, %2, !dbg !32
  call void @llvm.dbg.value(metadata <32 x i32> %7, metadata !20, metadata !DIExpression()), !dbg !32
  store <32 x i32> %7, <32 x i32>* %a, !dbg !33
  %8 = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %4, <32 x i1> %5), !dbg !34
  call void @llvm.dbg.value(metadata { <32 x i1>, i1 } %8, metadata !21, metadata !DIExpression()), !dbg !34
  ret void, !dbg !35
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "move-in-blocks.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
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
; CHECK-DAG: [[BR2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])

declare { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>)

declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "move-in-blocks.ll", directory: "/")
!2 = !{}
!3 = !{i32 13}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !17, !18, !20, !21}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty1024", size: 1024, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !12)
!12 = !DIBasicType(name: "ty768", size: 768, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !16)
!16 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !16)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 8, type: !19)
!19 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!20 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 10, type: !10)
!21 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 12, type: !22)
!22 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!23 = !DILocation(line: 1, column: 1, scope: !6)
!24 = !DILocation(line: 2, column: 1, scope: !6)
!25 = !DILocation(line: 3, column: 1, scope: !6)
!26 = !DILocation(line: 4, column: 1, scope: !6)
!27 = !DILocation(line: 5, column: 1, scope: !6)
!28 = !DILocation(line: 6, column: 1, scope: !6)
!29 = !DILocation(line: 7, column: 1, scope: !6)
!30 = !DILocation(line: 8, column: 1, scope: !6)
!31 = !DILocation(line: 9, column: 1, scope: !6)
!32 = !DILocation(line: 10, column: 1, scope: !6)
!33 = !DILocation(line: 11, column: 1, scope: !6)
!34 = !DILocation(line: 12, column: 1, scope: !6)
!35 = !DILocation(line: 13, column: 1, scope: !6)
