;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -genx-simplify -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimplify
; ------------------------------------------------
; This test checks that GenXSimplify pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; CHECK: void @test_simplify{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; Rd and wr calls are opt out, check that values are preserved
; CHECK: void @llvm.dbg.value(metadata <4 x i32> %{{[A-z0-9.]*}}, metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> %{{[A-z0-9.]*}}, metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]

define void @test_simplify(<4 x i32>* %a, <4 x i32> %b) !dbg !6 {
  %1 = load <4 x i32>, <4 x i32>* %a, !dbg !13
  call void @llvm.dbg.value(metadata <4 x i32> %1, metadata !9, metadata !DIExpression()), !dbg !13
  %2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %b, i32 1, i32 1, i32 1, i16 0, i32 1), !dbg !14
  call void @llvm.dbg.value(metadata <4 x i32> %2, metadata !11, metadata !DIExpression()), !dbg !14
  store <4 x i32> %2, <4 x i32>* %a, !dbg !15
  %3 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.v4i32.i16(<4 x i32> %1, <4 x i32> %2, i32 1, i32 1, i32 0, i16 0, i32 0, i1 false), !dbg !16
  call void @llvm.dbg.value(metadata <4 x i32> %3, metadata !12, metadata !DIExpression()), !dbg !16
  store <4 x i32> %3, <4 x i32>* %a, !dbg !17
  ret void, !dbg !18
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "intrinsic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_simplify", linkageName: "test_simplify", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

declare <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)

declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.v4i32.i16(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "intrinsic.ll", directory: "/")
!2 = !{}
!3 = !{i32 6}
!4 = !{i32 3}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_simplify", linkageName: "test_simplify", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 4, type: !10)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
!17 = !DILocation(line: 5, column: 1, scope: !6)
!18 = !DILocation(line: 6, column: 1, scope: !6)
