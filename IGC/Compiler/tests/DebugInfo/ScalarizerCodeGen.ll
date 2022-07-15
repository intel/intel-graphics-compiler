;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-scalarizer-in-codegen -S < %s | FileCheck %s
; ------------------------------------------------
; ScalarizerCodeGen
; ------------------------------------------------
; This test checks that ScalarizerCodeGen pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; Sanity checks (scope, allocas)
; CHECK: @test_scalarcg{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[A4_V:%[A-z0-9]*]] = alloca <4 x i8>{{.*}} !dbg [[A4_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <4 x i8>* [[A4_V]], metadata [[A4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[A4_LOC]]
; CHECK: [[A3_V:%[A-z0-9]*]] = alloca <3 x i32>{{.*}} !dbg [[A3_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <3 x i32>* [[A3_V]], metadata [[A3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[A3_LOC]]
; And operation:
; CHECK: %{{.*}} = and{{.*}} !dbg [[AND_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata <4 x i8> [[AND_V:%[0-9A-z]*]], metadata [[AND_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AND_LOC]]
; CHECK-DAG: [[AND_V]] = {{.*}} !dbg [[AND_LOC]]
; Or operation:
; CHECK: %{{.*}} = or{{.*}} !dbg [[OR_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata <3 x i32> [[OR_V:%[0-9A-z]*]], metadata [[OR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[OR_LOC]]
; CHECK-DAG: [[OR_V]] = {{.*}} !dbg [[OR_LOC]]

define void @test_scalarcg(<4 x i8> %src1, <3 x i32> %src2) !dbg !6 {
  %1 = alloca <4 x i8>, align 1, !dbg !16
  call void @llvm.dbg.value(metadata <4 x i8>* %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = alloca <3 x i32>, align 4, !dbg !17
  call void @llvm.dbg.value(metadata <3 x i32>* %2, metadata !11, metadata !DIExpression()), !dbg !17
  %3 = and <4 x i8> <i8 1, i8 2, i8 3, i8 4>, %src1, !dbg !18
  call void @llvm.dbg.value(metadata <4 x i8> %3, metadata !12, metadata !DIExpression()), !dbg !18
  %4 = or <3 x i32> <i32 13, i32 42, i32 17>, %src2, !dbg !19
  call void @llvm.dbg.value(metadata <3 x i32> %4, metadata !14, metadata !DIExpression()), !dbg !19
  store <4 x i8> %3, <4 x i8>* %1, !dbg !20
  store <3 x i32> %4, <3 x i32>* %2, !dbg !21
  ret void, !dbg !22
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ScalarizerCodeGen.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_scalarcg", linkageName: "test_scalarcg", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[A4_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[A4_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[A3_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[A3_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[AND_MD]] =  !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[AND_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[OR_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[OR_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ScalarizerCodeGen.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_scalarcg", linkageName: "test_scalarcg", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !15)
!15 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
