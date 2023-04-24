;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXLoadStoreLowering -enable-ldst-lowering=true -mattr=+ocl_runtime -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXLoadStoreLowering -enable-ldst-lowering=true -mattr=+ocl_runtime -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s -check-prefix=STRICT
; ------------------------------------------------
; GenXLoadStoreLowering
; ------------------------------------------------
; This test checks that GenXLoadStoreLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: void @test_lowerloadstore{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: bbi16:
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i16* [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[VAL2_V:%[A-z0-9.]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: scatter{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: bbi64:
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i64* [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i64 [[VAL4_V:%[A-z0-9.]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: scatter{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; CHECK: bbfptr:
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata void (i32*)** [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata void (i32*)* [[VAL6_V:%[A-z0-9.]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: scatter{{.*}}, !dbg [[STORE3_LOC:![0-9]*]]
;
; STRICT: bbi16:
; STRICT: , !dbg [[VAL1_LOC:![0-9]*]]
; STRICT: , !dbg [[VAL1_LOC]]
; STRICT: , !dbg [[VAL2_LOC:![0-9]*]]
; STRICT-COUNT-8: , !dbg [[VAL2_LOC]]
; STRICT: , !dbg [[STORE1_LOC:![0-9]*]]
; STRICT-COUNT-7: , !dbg [[STORE1_LOC]]
; STRICT: bbi64:
; STRICT: , !dbg [[VAL3_LOC:![0-9]*]]
; STRICT: , !dbg [[VAL3_LOC]]
; STRICT: , !dbg [[VAL4_LOC:![0-9]*]]
; STRICT-COUNT-6: , !dbg [[VAL4_LOC]]
; STRICT: , !dbg [[STORE2_LOC:![0-9]*]]
; STRICT-COUNT-5: , !dbg [[STORE2_LOC]]
; STRICT: bbfptr:
; STRICT: , !dbg [[VAL5_LOC:![0-9]*]]
; STRICT: , !dbg [[VAL5_LOC]]
; STRICT: , !dbg [[VAL6_LOC:![0-9]*]]
; STRICT-COUNT-7: , !dbg [[VAL6_LOC]]
; STRICT: , !dbg [[STORE3_LOC:![0-9]*]]
; STRICT-COUNT-6: , !dbg [[STORE3_LOC]]

define void @test_lowerloadstore(i32 %a) !dbg !6 {
bbi16:
  %0 = inttoptr i32 %a to i16*, !dbg !17
  call void @llvm.dbg.value(metadata i16* %0, metadata !9, metadata !DIExpression()), !dbg !17
  %1 = load i16, i16* %0, !dbg !18
  call void @llvm.dbg.value(metadata i16 %1, metadata !11, metadata !DIExpression()), !dbg !18
  store i16 %1, i16* %0, !dbg !19
  br label %bbi64, !dbg !20

bbi64:                                            ; preds = %bbi16
  %2 = inttoptr i32 %a to i64*, !dbg !21
  call void @llvm.dbg.value(metadata i64* %2, metadata !13, metadata !DIExpression()), !dbg !21
  %3 = load i64, i64* %2, !dbg !22
  call void @llvm.dbg.value(metadata i64 %3, metadata !14, metadata !DIExpression()), !dbg !22
  store i64 %3, i64* %2, !dbg !23
  br label %bbfptr, !dbg !24

bbfptr:                                           ; preds = %bbi64
  %4 = inttoptr i32 %a to void (i32*)**, !dbg !25
  call void @llvm.dbg.value(metadata void (i32*)** %4, metadata !15, metadata !DIExpression()), !dbg !25
  %5 = load void (i32*)*, void (i32*)** %4, !dbg !26
  call void @llvm.dbg.value(metadata void (i32*)* %5, metadata !16, metadata !DIExpression()), !dbg !26
  store void (i32*)* %5, void (i32*)** %4, !dbg !27
  ret void, !dbg !28
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXLoadStoreLowering.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_lowerloadstore", linkageName: "test_lowerloadstore", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])

; STRICT-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1
; STRICT-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1
; STRICT-DAG: [[STORE1_LOC]] = !DILocation(line: 3, column: 1
; STRICT-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1
; STRICT-DAG: [[VAL4_LOC]] = !DILocation(line: 6, column: 1
; STRICT-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1
; STRICT-DAG: [[VAL5_LOC]] = !DILocation(line: 9, column: 1
; STRICT-DAG: [[VAL6_LOC]] = !DILocation(line: 10, column: 1
; STRICT-DAG: [[STORE3_LOC]] = !DILocation(line: 11, column: 1

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXLoadStoreLowering.ll", directory: "/")
!2 = !{}
!3 = !{i32 12}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_lowerloadstore", linkageName: "test_lowerloadstore", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 5, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 6, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 9, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 10, type: !10)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
!26 = !DILocation(line: 10, column: 1, scope: !6)
!27 = !DILocation(line: 11, column: 1, scope: !6)
!28 = !DILocation(line: 12, column: 1, scope: !6)
