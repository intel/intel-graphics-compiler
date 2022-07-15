;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-registerestimator -S < %s | FileCheck %s
; ------------------------------------------------
; RegisterEstimator
; ------------------------------------------------
; This test checks that RegisterEstimator pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;
; This is analysis pass, check that debug is not modified
;
; CHECK: @test_register{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[ADD_V:%[A-z0-9]*]] = add <16 x i32>{{.*}} !dbg [[ADD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <16 x i32> [[ADD_V]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC]]
; CHECK: [[ALLOC_V:%[A-z0-9]*]] = alloca <16 x i32>{{.*}} !dbg [[ALLOC_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <16 x i32>* [[ALLOC_V]], metadata [[ALLOC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOC_LOC]]
; CHECK: store{{.*}} !dbg [[STORE_LOC:![0-9]*]]
; CHECK: [[LOAD_V:%[A-z0-9]*]] = load <16 x i32>{{.*}} !dbg [[LOAD_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <16 x i32> [[LOAD_V]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC]]
; CHECK: [[MUL_V:%[A-z0-9]*]] = mul <16 x i32>{{.*}} !dbg [[MUL_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <16 x i32> [[MUL_V]], metadata [[MUL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[MUL_LOC]]

define void @test_register(<16 x i32> %a, <16 x i32> %b) !dbg !6 {
  %1 = add <16 x i32> %a, %b, !dbg !15
  call void @llvm.dbg.value(metadata <16 x i32> %1, metadata !9, metadata !DIExpression()), !dbg !15
  %2 = alloca <16 x i32>, align 4, !dbg !16
  call void @llvm.dbg.value(metadata <16 x i32>* %2, metadata !11, metadata !DIExpression()), !dbg !16
  store <16 x i32> %b, <16 x i32>* %2, !dbg !17
  %3 = load <16 x i32>, <16 x i32>* %2, !dbg !18
  call void @llvm.dbg.value(metadata <16 x i32> %3, metadata !13, metadata !DIExpression()), !dbg !18
  %4 = mul <16 x i32> %3, %1, !dbg !19
  call void @llvm.dbg.value(metadata <16 x i32> %4, metadata !14, metadata !DIExpression()), !dbg !19
  store <16 x i32> %4, <16 x i32>* %2, !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "RegisterEstimator.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_register", linkageName: "test_register", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ALLOC_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[ALLOC_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[MUL_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[MUL_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "RegisterEstimator.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_register", linkageName: "test_register", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty512", size: 512, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocation(line: 1, column: 1, scope: !6)
!16 = !DILocation(line: 2, column: 1, scope: !6)
!17 = !DILocation(line: 3, column: 1, scope: !6)
!18 = !DILocation(line: 4, column: 1, scope: !6)
!19 = !DILocation(line: 5, column: 1, scope: !6)
!20 = !DILocation(line: 6, column: 1, scope: !6)
!21 = !DILocation(line: 7, column: 1, scope: !6)
