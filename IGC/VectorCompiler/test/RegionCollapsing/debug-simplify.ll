;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXRegionCollapsing
; ------------------------------------------------
; This test checks that GenXRegionCollapsing pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; CHECK: void @test_region{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = load {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; values %2, %3 and %4 are opt out, and can't be salvaged(vectors)
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[VAL5_V:%[A-z0-9.]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]

define void @test_region(<4 x i32>* %a, i16* %b) !dbg !6 {
  %1 = load <4 x i32>, <4 x i32>* %a, !dbg !17
  call void @llvm.dbg.value(metadata <4 x i32> %1, metadata !9, metadata !DIExpression()), !dbg !17
  %2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %1, i32 1, i32 1, i32 0, i16 0, i32 1), !dbg !18
  call void @llvm.dbg.value(metadata <4 x i32> %2, metadata !11, metadata !DIExpression()), !dbg !18
  %3 = trunc <4 x i32> %2 to <4 x i16>, !dbg !19
  call void @llvm.dbg.value(metadata <4 x i16> %3, metadata !12, metadata !DIExpression()), !dbg !19
  %4 = call <1 x i16> @llvm.genx.rdregioni.v1i16.v4i16.i16(<4 x i16> %3, i32 0, i32 1, i32 0, i16 0, i32 1), !dbg !20
  call void @llvm.dbg.value(metadata <1 x i16> %4, metadata !14, metadata !DIExpression()), !dbg !20
  %5 = extractelement <1 x i16> %4, i32 0, !dbg !21
  call void @llvm.dbg.value(metadata i16 %5, metadata !16, metadata !DIExpression()), !dbg !21
  store i16 %5, i16* %b, !dbg !22
  ret void, !dbg !23
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "simplify.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_region", linkageName: "test_region", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

declare <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)

declare <1 x i16> @llvm.genx.rdregioni.v1i16.v4i16.i16(<4 x i16>, i32, i32, i32, i16, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "simplify.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_region", linkageName: "test_region", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !15)
!15 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !15)
!17 = !DILocation(line: 1, column: 1, scope: !6)
!18 = !DILocation(line: 2, column: 1, scope: !6)
!19 = !DILocation(line: 3, column: 1, scope: !6)
!20 = !DILocation(line: 4, column: 1, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
!22 = !DILocation(line: 6, column: 1, scope: !6)
!23 = !DILocation(line: 7, column: 1, scope: !6)
