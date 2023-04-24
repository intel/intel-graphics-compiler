;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXFuncBaling -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXGroupBalingWrapper -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXBaling
; ------------------------------------------------
; This test checks that GenXFuncBaling and GenXGroupBaling passes follow
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.


; CHECK: define void @test_bale{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32>* [[VAL1_V:%[A-z0-9]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <8 x i32>* [[VAL2_V:%[A-z0-9]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32> [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <8 x i32> [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK: store {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: store {{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; CHECK: bb1:
; CHECK: store {{.*}}, !dbg [[STORE3_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata <16 x i32>* [[VAL7_V:%[A-z0-9]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: store {{.*}}, !dbg [[STORE4_LOC:![0-9]*]]

@b = common global i32* null, align 8

define void @test_bale(<32 x i32>* %a) #0 !dbg !6 {
entry:
  %0 = bitcast <32 x i32>* %a to <16 x i32>*, !dbg !19
  call void @llvm.dbg.value(metadata <16 x i32>* %0, metadata !9, metadata !DIExpression()), !dbg !19
  %1 = bitcast <16 x i32>* %0 to <8 x i32>*, !dbg !20
  call void @llvm.dbg.value(metadata <8 x i32>* %1, metadata !11, metadata !DIExpression()), !dbg !20
  %2 = load <16 x i32>, <16 x i32>* %0, !dbg !21
  call void @llvm.dbg.value(metadata <16 x i32> %2, metadata !12, metadata !DIExpression()), !dbg !21
  %3 = load <8 x i32>, <8 x i32>* %1, !dbg !22
  call void @llvm.dbg.value(metadata <8 x i32> %3, metadata !14, metadata !DIExpression()), !dbg !22
  store <16 x i32> %2, <16 x i32>* %0, !dbg !23
  store <8 x i32> %3, <8 x i32>* %1, !dbg !24
  br label %bb1, !dbg !25

bb1:                                              ; preds = %entry
  %4 = bitcast <8 x i32>* %1 to i32*, !dbg !26
  call void @llvm.dbg.value(metadata i32* %4, metadata !16, metadata !DIExpression()), !dbg !26
  store i32* %4, i32** @b, !dbg !27
  %5 = load i32*, i32** @b, !dbg !28
  call void @llvm.dbg.value(metadata i32* %5, metadata !17, metadata !DIExpression()), !dbg !28
  %6 = bitcast i32* %5 to <16 x i32>*, !dbg !29
  call void @llvm.dbg.value(metadata <16 x i32>* %6, metadata !18, metadata !DIExpression()), !dbg !29
  store <16 x i32> %2, <16 x i32>* %6, !dbg !30
  ret void, !dbg !31
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "bitcast.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_bale", linkageName: "test_bale", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE4_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "bitcast.ll", directory: "/")
!2 = !{}
!3 = !{i32 13}
!4 = !{i32 7}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_bale", linkageName: "test_bale", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !16, !17, !18}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !15)
!15 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 8, type: !10)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 10, type: !10)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 11, type: !10)
!19 = !DILocation(line: 1, column: 1, scope: !6)
!20 = !DILocation(line: 2, column: 1, scope: !6)
!21 = !DILocation(line: 3, column: 1, scope: !6)
!22 = !DILocation(line: 4, column: 1, scope: !6)
!23 = !DILocation(line: 5, column: 1, scope: !6)
!24 = !DILocation(line: 6, column: 1, scope: !6)
!25 = !DILocation(line: 7, column: 1, scope: !6)
!26 = !DILocation(line: 8, column: 1, scope: !6)
!27 = !DILocation(line: 9, column: 1, scope: !6)
!28 = !DILocation(line: 10, column: 1, scope: !6)
!29 = !DILocation(line: 11, column: 1, scope: !6)
!30 = !DILocation(line: 12, column: 1, scope: !6)
!31 = !DILocation(line: 13, column: 1, scope: !6)
