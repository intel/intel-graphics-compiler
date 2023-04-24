;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXInlineAsmLowering -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXInlineAsmLowering
; ------------------------------------------------
; This test checks that GenXInlineAsmLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; CHECK: void @test_lowerinline{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = call i32{{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: call i1{{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = zext{{.*}}, !dbg [[VAL2_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = call <8 x i32>{{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <8 x i32> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]

define void @test_lowerinline(i32* %a, i32* %b, <8 x i32>* %c) !dbg !6 {
  %1 = load i32, i32* %a, !dbg !17
  call void @llvm.dbg.value(metadata i32 %1, metadata !9, metadata !DIExpression()), !dbg !17
  %2 = load i32, i32* %b, !dbg !18
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !18
  %3 = call i32 asm "mul (M1,1) %0(0,0)<1> %1(0,0)<1;1,0> %2(0,0)<1;1,0>", "=r,r,r"(i32 %1, i32 %2), !dbg !19
  call void @llvm.dbg.value(metadata i32 %3, metadata !12, metadata !DIExpression()), !dbg !19
  store i32 %3, i32* %a, !dbg !20
  %4 = load <8 x i32>, <8 x i32>* %c, !dbg !21
  call void @llvm.dbg.value(metadata <8 x i32> %4, metadata !13, metadata !DIExpression()), !dbg !21
  %5 = call i32 asm "cmp.lt (M1_NM, 8) %0 %1(0,0)<0;1,0> 0x3:ud", "=@2cr,r"(<8 x i32> %4), !dbg !22
  call void @llvm.dbg.value(metadata i32 %5, metadata !15, metadata !DIExpression()), !dbg !22
  store i32 %5, i32* %a, !dbg !23
  %6 = call <8 x i32> asm "%1 sel (M1_NM, 8) %0(0,0)<1> %1(0,0)<0;1,0> 0x3:ud", "=r,@2cr,r"(<8 x i32> %4, <8 x i32> %4), !dbg !24
  call void @llvm.dbg.value(metadata <8 x i32> %6, metadata !16, metadata !DIExpression()), !dbg !24
  store <8 x i32> %6, <8 x i32>* %c, !dbg !25
  store <8 x i32> %4, <8 x i32>* %c, !dbg !26
  ret void, !dbg !27
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXInlineAsmLowering.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_lowerinline", linkageName: "test_lowerinline", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXInlineAsmLowering.ll", directory: "/")
!2 = !{}
!3 = !{i32 11}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_lowerinline", linkageName: "test_lowerinline", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !15, !16}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !14)
!14 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 8, type: !14)
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
