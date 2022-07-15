;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --half-promotion  -S < %s | FileCheck %s
; ------------------------------------------------
; HalfPromotion
; ------------------------------------------------
; This test checks that HalfPromotion pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_half
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata half [[FADD_V:%[0-9]*]], metadata [[FADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FADD_LOC:![0-9]*]]
; CHECK-DAG: [[FADD_V]] = {{.*}}, !dbg [[FADD_LOC:![0-9]*]]
;
; CHECK-DAG: @llvm.dbg.value(metadata half [[FMUL_V:%[0-9]*]], metadata [[FMUL_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FMUL_LOC:![0-9]*]]
; CHECK-DAG: [[FMUL_V]] = {{.*}}, !dbg [[FMUL_LOC:![0-9]*]]
;
; CHECK-DAG: @llvm.dbg.value(metadata half [[UITO_V:%[0-9]*]], metadata [[UITO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[UITO_LOC:![0-9]*]]
; CHECK-DAG: [[UITO_V]] = {{.*}}, !dbg [[UITO_LOC:![0-9]*]]
;
; CHECK-DAG: @llvm.dbg.value(metadata half [[SITO_V:%[0-9]*]], metadata [[SITO_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SITO_LOC:![0-9]*]]
; CHECK-DAG: [[SITO_V]] = {{.*}}, !dbg [[SITO_LOC:![0-9]*]]
;
; CHECK-DAG: @llvm.dbg.value(metadata i16 [[TOUI_V:%[0-9]*]], metadata [[TOUI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[TOUI_LOC:![0-9]*]]
; CHECK-DAG: [[TOUI_V]] = {{.*}}, !dbg [[TOUI_LOC:![0-9]*]]
;
; CHECK-DAG: @llvm.dbg.value(metadata i16 [[TOSI_V:%[0-9]*]], metadata [[TOSI_MD:![0-9]*]], metadata !DIExpression()), !dbg [[TOSI_LOC:![0-9]*]]
; CHECK-DAG: [[TOSI_V]] = {{.*}}, !dbg [[TOSI_LOC:![0-9]*]]

define void @test_half(half %src1, half %src2, i32 %src3) !dbg !6 {
  %1 = fadd half %src1, %src2, !dbg !16
  call void @llvm.dbg.value(metadata half %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = fmul half %src2, %1, !dbg !17
  call void @llvm.dbg.value(metadata half %2, metadata !11, metadata !DIExpression()), !dbg !17
  %3 = uitofp i32 %src3 to half, !dbg !18
  call void @llvm.dbg.value(metadata half %3, metadata !12, metadata !DIExpression()), !dbg !18
  %4 = sitofp i32 %src3 to half, !dbg !19
  call void @llvm.dbg.value(metadata half %4, metadata !13, metadata !DIExpression()), !dbg !19
  %5 = fptoui half %4 to i16, !dbg !20
  call void @llvm.dbg.value(metadata i16 %5, metadata !14, metadata !DIExpression()), !dbg !20
  %6 = fptosi half %3 to i16, !dbg !21
  call void @llvm.dbg.value(metadata i16 %6, metadata !15, metadata !DIExpression()), !dbg !21
  ret void, !dbg !22
}
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "binary.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_half", linkageName: "test_half", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[FADD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[FADD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[FMUL_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[FMUL_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[UITO_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[UITO_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SITO_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[SITO_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[TOUI_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[TOUI_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[TOSI_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[TOSI_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "binary.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 6}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_half", linkageName: "test_half", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
