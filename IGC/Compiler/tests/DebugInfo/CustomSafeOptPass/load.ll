;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass
; ------------------------------------------------
; This test checks that CustomSafeOptPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_customsafe{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; select values are dead skip them
; gep should have line preserved but value is unsalvageable since there are 2 now
; CHECK: %{{.*}} = {{.*}}, !dbg [[GEP1_LOC:![0-9]*]]
; CHECK: %{{.*}} = {{.*}}, !dbg [[GEP1_LOC]]
; load value is now select
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[LOAD1_V:%[A-z0-9]*]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD1_V]] = {{.*}}, !dbg [[LOAD1_LOC]]
; CHECK: store i32 [[LOAD1_V]]{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]

; CHECK: %{{.*}} = {{.*}}, !dbg [[GEP2_LOC:![0-9]*]]
; CHECK: %{{.*}} = {{.*}}, !dbg [[GEP2_LOC]]
; load value is now select
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[LOAD2_V:%[A-z0-9]*]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD2_V]] = {{.*}}, !dbg [[LOAD2_LOC]]
; CHECK: store i32 [[LOAD2_V]]{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]

define spir_kernel void @test_customsafe([7 x [3 x i32]]* %a, i32* %b, i1 %c) !dbg !6 {
entry:
  %0 = select i1 %c, i32 4, i32 5, !dbg !17
  call void @llvm.dbg.value(metadata i32 %0, metadata !9, metadata !DIExpression()), !dbg !17
  %1 = select i1 %c, i32 0, i32 1, !dbg !18
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !18
  %2 = getelementptr [7 x [3 x i32]], [7 x [3 x i32]]* %a, i32 0, i32 0, i32 %1, !dbg !19
  call void @llvm.dbg.value(metadata i32* %2, metadata !12, metadata !DIExpression()), !dbg !19
  %3 = load i32, i32* %2, !dbg !20
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !20
  store i32 %3, i32* %b, !dbg !21
  %4 = getelementptr [7 x [3 x i32]], [7 x [3 x i32]]* %a, i32 0, i32 %0, i32 1, !dbg !22
  call void @llvm.dbg.value(metadata i32* %4, metadata !15, metadata !DIExpression()), !dbg !22
  %5 = load i32, i32* %4, !dbg !23
  call void @llvm.dbg.value(metadata i32 %5, metadata !16, metadata !DIExpression()), !dbg !23
  store i32 %5, i32* %b, !dbg !24
  ret void, !dbg !25
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "load.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[GEP1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[GEP2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "load.ll", directory: "/")
!2 = !{}
!3 = !{i32 9}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_customsafe", linkageName: "test_customsafe", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !13)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !10)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
!24 = !DILocation(line: 8, column: 1, scope: !6)
!25 = !DILocation(line: 9, column: 1, scope: !6)
