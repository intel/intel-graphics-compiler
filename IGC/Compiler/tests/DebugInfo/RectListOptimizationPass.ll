;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt  -igc-rectlist-opt -inputgs -S < %s | FileCheck %s
; ------------------------------------------------
; RectListOptimizationPass
; ------------------------------------------------
; This test checks that RectListOptimizationPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------
; This is essentially analysis path with no actual llvm IR changes

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.OUTPUTGS{{.*}}, !dbg [[CALL1_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.OUTPUTGS{{.*}}, !dbg [[CALL2_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.OUTPUTGS{{.*}}, !dbg [[CALL3_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.OUTPUTGS{{.*}}, !dbg [[CALL4_LOC:![0-9]*]]

@GsMaxOutputVertices = global i32 4
@GsOutputPrimitiveTopology = global i32 0
@SamplerCount = global i32 0
@GsInputPrimitiveType = global i32 0
@GsMaxInputAttributeCount = global i32 0
@CutEncountered = global i32 1
@DefaultStreamID = global i32 0

define void @test(i32 %src) !dbg !12 {
  call void @llvm.genx.GenISA.OUTPUTGS(float 1.000000e+00, float 2.000000e+00, float 6.000000e+00, float 4.200000e+01, i32 1, i32 2, i32 3, i32 15), !dbg !14
  call void @llvm.genx.GenISA.OUTPUTGS(float 1.000000e+00, float 2.000000e+00, float 6.000000e+00, float 4.200000e+01, i32 1, i32 2, i32 2, i32 15), !dbg !15
  call void @llvm.genx.GenISA.OUTPUTGS(float 3.000000e+00, float 4.000000e+00, float 6.000000e+00, float 4.200000e+01, i32 1, i32 2, i32 1, i32 15), !dbg !16
  call void @llvm.genx.GenISA.OUTPUTGS(float 3.000000e+00, float 4.000000e+00, float 6.000000e+00, float 4.200000e+01, i32 1, i32 2, i32 0, i32 15), !dbg !17
  ret void, !dbg !18
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "RectListOptimizationPass.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[CALL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])


declare void @llvm.genx.GenISA.OUTPUTGS(float, float, float, float, i32, i32, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.debugify = !{!4, !5, !6, !7}
!llvm.module.flags = !{!8}
!llvm.dbg.cu = !{!9}

!0 = !{void (i32)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{i32 7}
!5 = !{i32 3}
!6 = !{i32 5}
!7 = !{i32 0}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DICompileUnit(language: DW_LANG_C, file: !10, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !11)
!10 = !DIFile(filename: "RectListOptimizationPass.ll", directory: "/")
!11 = !{}
!12 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !10, line: 1, type: !13, scopeLine: 1, unit: !9, retainedNodes: !11)
!13 = !DISubroutineType(types: !11)
!14 = !DILocation(line: 1, column: 1, scope: !12)
!15 = !DILocation(line: 2, column: 1, scope: !12)
!16 = !DILocation(line: 3, column: 1, scope: !12)
!17 = !DILocation(line: 4, column: 1, scope: !12)
!18 = !DILocation(line: 5, column: 1, scope: !12)
