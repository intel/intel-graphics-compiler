;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPatternMatch
; ------------------------------------------------
; This test checks that GenXPatternMatch pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.


; CHECK: void @test_patternmatch{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <2 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; lost const vector value, can't be salvaged
; CHECK-DAG: void @llvm.dbg.value(metadata <2 x i1> [[VAL3_V:%[A-z0-9.]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <2 x i32> [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; optimized out,  value rauw'd
; CHECK: void @llvm.dbg.value(metadata <2 x i1> {{%[A-z0-9.]*}}, metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <2 x i1> {{%[A-z0-9.]*}}, metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: [[VAL7_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <2 x i32> [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; CHECK: [[VAL8_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <2 x i1> [[VAL8_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC]]
; value optimized out
; CHECK: [[VAL10_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL10_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL10_V]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC]]
; CHECK: [[VAL11_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL11_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <2 x i32> [[VAL11_V]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC]]


define void @test_patternmatch(<2 x i32>* %a) !dbg !6 {
entry:
  %0 = load <2 x i32>, <2 x i32>* %a, !dbg !23
  call void @llvm.dbg.value(metadata <2 x i32> %0, metadata !9, metadata !DIExpression()), !dbg !23
  %1 = and <2 x i32> %0, <i32 65535, i32 65535>, !dbg !24
  call void @llvm.dbg.value(metadata <2 x i32> %1, metadata !11, metadata !DIExpression()), !dbg !24
  %2 = icmp ult <2 x i32> %1, <i32 13, i32 14>, !dbg !25
  call void @llvm.dbg.value(metadata <2 x i1> %2, metadata !12, metadata !DIExpression()), !dbg !25
  %3 = select <2 x i1> %2, <2 x i32> %0, <2 x i32> <i32 0, i32 1>, !dbg !26
  call void @llvm.dbg.value(metadata <2 x i32> %3, metadata !14, metadata !DIExpression()), !dbg !26
  store <2 x i32> %3, <2 x i32>* %a, !dbg !27
  %4 = select <2 x i1> %2, <2 x i1> <i1 true, i1 true>, <2 x i1> zeroinitializer, !dbg !28
  call void @llvm.dbg.value(metadata <2 x i1> %4, metadata !15, metadata !DIExpression()), !dbg !28
  %5 = icmp ne <2 x i1> %4, zeroinitializer, !dbg !29
  call void @llvm.dbg.value(metadata <2 x i1> %5, metadata !16, metadata !DIExpression()), !dbg !29
  %6 = select <2 x i1> %5, <2 x i32> %0, <2 x i32> <i32 0, i32 1>, !dbg !30
  call void @llvm.dbg.value(metadata <2 x i32> %6, metadata !17, metadata !DIExpression()), !dbg !30
  store <2 x i32> %6, <2 x i32>* %a, !dbg !31
  %7 = icmp ult <2 x i32> %0, <i32 13, i32 15>, !dbg !32
  call void @llvm.dbg.value(metadata <2 x i1> %7, metadata !18, metadata !DIExpression()), !dbg !32
  %8 = bitcast <2 x i1> %7 to i2, !dbg !33
  call void @llvm.dbg.value(metadata i2 %8, metadata !19, metadata !DIExpression()), !dbg !33
  %9 = icmp eq i2 %8, 0, !dbg !34
  call void @llvm.dbg.value(metadata i1 %9, metadata !21, metadata !DIExpression()), !dbg !34
  %10 = select i1 %9, <2 x i32> %0, <2 x i32> <i32 0, i32 1>, !dbg !35
  call void @llvm.dbg.value(metadata <2 x i32> %10, metadata !22, metadata !DIExpression()), !dbg !35
  store <2 x i32> %10, <2 x i32>* %a, !dbg !36
  ret void, !dbg !37
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "icmp.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "icmp.ll", directory: "/")
!2 = !{}
!3 = !{i32 15}
!4 = !{i32 11}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_patternmatch", linkageName: "test_patternmatch", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15, !16, !17, !18, !19, !21, !22}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !13)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !13)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 8, type: !10)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 10, type: !13)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 11, type: !20)
!20 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 12, type: !20)
!22 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 13, type: !10)
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
!36 = !DILocation(line: 14, column: 1, scope: !6)
!37 = !DILocation(line: 15, column: 1, scope: !6)
