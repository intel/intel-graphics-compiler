;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;
; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXLegalization
; ------------------------------------------------
; This test checks that GenXLegalization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; Current test check corectness of legalization bale with gstore.
;

; CHECK: void @test_transform{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <128 x i8> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value({{.*}}, metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: {{.*}} = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK: store {{.*}}<128 x i8> {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]

@global_vec = internal global <128 x i8> undef, align 8

define void @test_transform(<128 x i8>* %a) !dbg !6 {
entry:
  %0 = load <128 x i8>, <128 x i8>* %a, !dbg !12
  call void @llvm.dbg.value(metadata <128 x i8> %0, metadata !9, metadata !DIExpression()), !dbg !12
  %1 = call <128 x i8> @llvm.genx.rdregioni.v128i8.v128i8.i16(<128 x i8> %0, i32 1, i32 1, i32 0, i16 16, i32 0), !dbg !13
  call void @llvm.dbg.value(metadata <128 x i8> %1, metadata !9, metadata !DIExpression()), !dbg !13
  store <128 x i8> %1, <128 x i8>* @global_vec, !dbg !14
  ret void, !dbg !15
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "gstore.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_transform", linkageName: "test_transform", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

declare <128 x i8> @llvm.genx.rdregioni.v128i8.v128i8.i16(<128 x i8>, i32, i32, i32, i16, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "gstore.ll", directory: "/")
!2 = !{}
!3 = !{i32 4}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_transform", linkageName: "test_transform", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty1024", size: 1024, encoding: DW_ATE_unsigned)
!12 = !DILocation(line: 1, column: 1, scope: !6)
!13 = !DILocation(line: 2, column: 1, scope: !6)
!14 = !DILocation(line: 3, column: 1, scope: !6)
!15 = !DILocation(line: 4, column: 1, scope: !6)

