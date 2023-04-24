;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXPacketize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPacketize
; ------------------------------------------------
; This test checks that GenXPacketize pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; Entry part:

define void @test_packetize(float %a) !dbg !6 {
  %1 = insertelement <8 x float> zeroinitializer, float %a, i32 2, !dbg !11
  call void @llvm.dbg.value(metadata <8 x float> %1, metadata !9, metadata !DIExpression()), !dbg !11
  call void @test_vectorize(<8 x float> %1), !dbg !12
  ret void, !dbg !13
}

; Modified part:
; Values are not salavageble

; CHECK: define internal void @test_vectorize{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.value
; CHECK: [[VAL4_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]

define internal void @test_vectorize(<8 x float> %a) #0 !dbg !14 {
entry:
  %0 = extractelement <8 x float> %a, i32 2, !dbg !21
  call void @llvm.dbg.value(metadata float %0, metadata !16, metadata !DIExpression()), !dbg !21
  %1 = call float @llvm.sqrt.f32(float %0), !dbg !22
  call void @llvm.dbg.value(metadata float %1, metadata !18, metadata !DIExpression()), !dbg !22
  %2 = call float @llvm.log2.f32(float %0), !dbg !23
  call void @llvm.dbg.value(metadata float %2, metadata !19, metadata !DIExpression()), !dbg !23
  %3 = call float @llvm.exp2.f32(float %0), !dbg !24
  call void @llvm.dbg.value(metadata float %3, metadata !20, metadata !DIExpression()), !dbg !24
  ret void, !dbg !25
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXPacketize.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vectorize", linkageName: "test_vectorize", scope: null, file: [[FILE]], line: 4
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare float @llvm.sqrt.f32(float) #1

; Function Attrs: nounwind readnone speculatable
declare float @llvm.exp2.f32(float) #1

; Function Attrs: nounwind readnone speculatable
declare float @llvm.log2.f32(float) #1

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxSIMT"="8" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXPacketize.ll", directory: "/")
!2 = !{}
!3 = !{i32 8}
!4 = !{i32 5}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_packetize", linkageName: "test_packetize", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
!13 = !DILocation(line: 3, column: 1, scope: !6)
!14 = distinct !DISubprogram(name: "test_vectorize", linkageName: "test_vectorize", scope: null, file: !1, line: 4, type: !7, scopeLine: 4, unit: !0, retainedNodes: !15)
!15 = !{!16, !18, !19, !20}
!16 = !DILocalVariable(name: "2", scope: !14, file: !1, line: 4, type: !17)
!17 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "3", scope: !14, file: !1, line: 5, type: !17)
!19 = !DILocalVariable(name: "4", scope: !14, file: !1, line: 6, type: !17)
!20 = !DILocalVariable(name: "5", scope: !14, file: !1, line: 7, type: !17)
!21 = !DILocation(line: 4, column: 1, scope: !14)
!22 = !DILocation(line: 5, column: 1, scope: !14)
!23 = !DILocation(line: 6, column: 1, scope: !14)
!24 = !DILocation(line: 7, column: 1, scope: !14)
!25 = !DILocation(line: 8, column: 1, scope: !14)
