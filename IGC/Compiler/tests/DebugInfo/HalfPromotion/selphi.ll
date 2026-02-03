;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --half-promotion  -S < %s | FileCheck %s
; ------------------------------------------------
; HalfPromotion
; ------------------------------------------------
; This test checks that HalfPromotion pass follows
; 'How to Update Debug Info' llvm guideline.

; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_half
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata i1 [[FCMP_V:%[0-9]*]], metadata [[FCMP_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FCMP_LOC:![0-9]*]]
; CHECK-DAG: [[FCMP_V]] = {{.*}}, !dbg [[FCMP_LOC:![0-9]*]]

; CHECK-DAG: @llvm.dbg.value(metadata half [[COS_V:%[0-9]*]], metadata [[COS_MD:![0-9]*]], metadata !DIExpression()), !dbg [[COS_LOC:![0-9]*]]
; CHECK-DAG: [[COS_V]] = {{.*}}, !dbg [[COS_LOC:![0-9]*]]

; CHECK-DAG: @llvm.dbg.value(metadata half [[WAVE_V:%[0-9]*]], metadata [[WAVE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[WAVE_LOC:![0-9]*]]
; CHECK-DAG: [[WAVE_V]] = {{.*}}, !dbg [[WAVE_LOC:![0-9]*]]

; CHECK-DAG: @llvm.dbg.value(metadata half [[SELECT_V:%[0-9]*]], metadata [[SELECT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SELECT_LOC:![0-9]*]]
; CHECK-DAG: [[SELECT_V]] = {{.*}}, !dbg [[SELECT_LOC:![0-9]*]]

; CHECK-DAG: @llvm.dbg.value(metadata half [[PHI_V:%[0-9]*]], metadata [[PHI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PHI_LOC:![0-9]*]]
; CHECK-DAG: [[PHI_V]] = {{.*}}, !dbg [[PHI_LOC:![0-9]*]]


define void @test_half(half %src1, half %src2, half* %dst) !dbg !6 {
entry:
  %0 = fcmp one half %src1, %src2, !dbg !16
  call void @llvm.dbg.value(metadata i1 %0, metadata !9, metadata !DIExpression()), !dbg !16
  %1 = call half @llvm.cos.f16(half %src1), !dbg !17
  call void @llvm.dbg.value(metadata half %1, metadata !11, metadata !DIExpression()), !dbg !17
  %2 = call half @llvm.genx.GenISA.WaveAll.f16(half 0xH4000, i8 1, i1 true, i32 0), !dbg !18
  call void @llvm.dbg.value(metadata half %2, metadata !13, metadata !DIExpression()), !dbg !18
  br i1 %0, label %var1, label %var2, !dbg !19

var1:                                             ; preds = %entry
  %3 = select i1 %0, half %1, half %2, !dbg !20
  call void @llvm.dbg.value(metadata half %3, metadata !14, metadata !DIExpression()), !dbg !20
  br label %var2, !dbg !21

var2:                                             ; preds = %var1, %entry
  %4 = phi half [ 0xH3C00, %entry ], [ %3, %var1 ], !dbg !22
  call void @llvm.dbg.value(metadata half %4, metadata !15, metadata !DIExpression()), !dbg !22
  ret void, !dbg !23
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "selphi.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_half", linkageName: "test_half", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[FCMP_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[FCMP_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[COS_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[COS_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[WAVE_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[WAVE_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SELECT_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[SELECT_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PHI_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[PHI_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare half @llvm.cos.f16(half) #0

declare half @llvm.genx.GenISA.WaveAll.f16(half, i8, i1, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "selphi.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_half", linkageName: "test_half", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 5, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 7, type: !12)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
!23 = !DILocation(line: 8, column: 1, scope: !6)
