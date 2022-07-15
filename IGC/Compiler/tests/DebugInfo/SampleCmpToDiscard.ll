;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-discard-samplecmp -inputps -S < %s | FileCheck %s
; ------------------------------------------------
; SampleCmpToDiscard
; ------------------------------------------------
; This test checks that SampleCmpToDiscard pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
;
; This is essentially analisys pass, thus check that nothing was modified


; CHECK: @test_samplecmp{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[SAMPLE_V:%[A-z0-9]*]] = call <4 x float>{{.*}} !dbg [[SAMPLE_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata <4 x float> [[SAMPLE_V]], metadata [[SAMPLE_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SAMPLE_LOC]]
; CHECK: [[F1_V:%[A-z0-9]*]] = extractelement <4 x float>{{.*}} !dbg [[F1_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float [[F1_V]], metadata [[F1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[F1_LOC]]
; CHECK: [[F2_V:%[A-z0-9]*]] = fsub float{{.*}} !dbg [[F2_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float [[F2_V]], metadata [[F2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[F2_LOC]]
; CHECK: [[F3_V:%[A-z0-9]*]] = fmul float{{.*}} !dbg [[F3_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float [[F3_V]], metadata [[F3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[F3_LOC]]
; CHECK: [[F4_V:%[A-z0-9]*]] = fadd float{{.*}} !dbg [[F4_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata float [[F4_V]], metadata [[F4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[F4_LOC]]
; CHECK: call void @llvm.genx.GenISA.RTWrite.f32(float [[F4_V]]{{.*}} !dbg [[RT_LOC:![0-9]*]]

define void @test_samplecmp(float %src1, float %src2, float %src3) !dbg !6 {
  %1 = call <4 x float> @llvm.genx.GenISA.sampleBCptr.v4f32.f32.p196609i8.p524293i8(float %src1, float 0.000000e+00, float %src2, float %src3, float 0.000000e+00, float 0.000000e+00, i8 addrspace(196609)* null, i8 addrspace(524293)* inttoptr (i64 5 to i8 addrspace(524293)*), i32 0, i32 0, i32 0), !dbg !16
  call void @llvm.dbg.value(metadata <4 x float> %1, metadata !9, metadata !DIExpression()), !dbg !16
  %2 = extractelement <4 x float> %1, i32 0, !dbg !17
  call void @llvm.dbg.value(metadata float %2, metadata !11, metadata !DIExpression()), !dbg !17
  %3 = fsub float 1.000000e+00, %2, !dbg !18
  call void @llvm.dbg.value(metadata float %3, metadata !13, metadata !DIExpression()), !dbg !18
  %4 = fmul float %3, 2.500000e-01, !dbg !19
  call void @llvm.dbg.value(metadata float %4, metadata !14, metadata !DIExpression()), !dbg !19
  %5 = fadd float %4, %2, !dbg !20
  call void @llvm.dbg.value(metadata float %5, metadata !15, metadata !DIExpression()), !dbg !20
  call void @llvm.genx.GenISA.RTWrite.f32(float %5, float %5, i1 true, float %5, float %5, float %5, float %5, float %5, float %5, i32 0, i32 0, i1 false, i1 false, i1 false, i1 false, i32 0), !dbg !21
  ret void, !dbg !22
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "SampleCmpToDiscard.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_samplecmp", linkageName: "test_samplecmp", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[SAMPLE_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SAMPLE_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[F1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[F1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[F2_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[F2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[F3_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[F3_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[F4_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[F4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[RT_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

declare void @llvm.genx.GenISA.RTWrite.f32(float, float, i1, float, float, float, float, float, float, i32, i32, i1, i1, i1, i1, i32)

declare <4 x float> @llvm.genx.GenISA.sampleBCptr.v4f32.f32.p196609i8.p524293i8(float, float, float, float, float, float, i8 addrspace(196609)*, i8 addrspace(524293)*, i32, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "SampleCmpToDiscard.ll", directory: "/")
!2 = !{}
!3 = !{i32 7}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_samplecmp", linkageName: "test_samplecmp", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)
!16 = !DILocation(line: 1, column: 1, scope: !6)
!17 = !DILocation(line: 2, column: 1, scope: !6)
!18 = !DILocation(line: 3, column: 1, scope: !6)
!19 = !DILocation(line: 4, column: 1, scope: !6)
!20 = !DILocation(line: 5, column: 1, scope: !6)
!21 = !DILocation(line: 6, column: 1, scope: !6)
!22 = !DILocation(line: 7, column: 1, scope: !6)
