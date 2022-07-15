;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; ------------------------------------------------
; RUN: igc_opt --igc-gep-lowering -S < %s | FileCheck %s
; ------------------------------------------------
; GEPLowering
; ------------------------------------------------
; This test checks that GEPLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK:  spir_kernel void @test_gep
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK-DAG: void @llvm.dbg.declare(metadata <4 x float>* [[GEP1_V:%[A-z0-9]*]], metadata [[GEP1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEP1_LOC:![0-9]*]]
; CHECK-DAG: [[GEP1_V]] = {{.*}}, !dbg [[GEP1_LOC]]
; CHECK: load <4 x float>{{.*}}, !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.declare(metadata i32* [[GEP2_V:%[A-z0-9]*]], metadata [[GEP2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEP2_LOC:![0-9]*]]
; CHECK-DAG: [[GEP2_V]] = {{.*}}, !dbg [[GEP2_LOC]]
; CHECK: load i32{{.*}}, !dbg [[LOAD2_LOC:![0-9]*]]

%struct._test = type { i32, <4 x float>, [2 x i64] }

define spir_kernel void @test_gep(%struct._test* byval(%struct._test) %src, i32 %iptr) !dbg !10 {
  %temp = alloca %struct._test, align 16, !dbg !22
  call void @llvm.dbg.value(metadata %struct._test* %temp, metadata !13, metadata !DIExpression()), !dbg !22
  %a = getelementptr inbounds %struct._test, %struct._test* %temp, i32 1, i32 1, !dbg !23
  call void @llvm.dbg.declare(metadata <4 x float>* %a, metadata !15, metadata !DIExpression()), !dbg !23
  %1 = load <4 x float>, <4 x float>* %a, align 16, !dbg !24
  call void @llvm.dbg.value(metadata <4 x float> %1, metadata !16, metadata !DIExpression()), !dbg !24
  %ptr = inttoptr i32 %iptr to i32*, !dbg !25
  call void @llvm.dbg.value(metadata i32* %ptr, metadata !18, metadata !DIExpression()), !dbg !25
  %b = getelementptr inbounds i32, i32* %ptr, i32 2, !dbg !26
  call void @llvm.dbg.declare(metadata i32* %b, metadata !19, metadata !DIExpression()), !dbg !26
  %2 = load i32, i32* %b, align 16, !dbg !27
  call void @llvm.dbg.value(metadata i32 %2, metadata !20, metadata !DIExpression()), !dbg !27
  ret void, !dbg !28
}
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_gep", linkageName: "test_gep", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[GEP1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[GEP1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[GEP2_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[GEP2_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (%struct._test*, i32)* @test_gep, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "basic.ll", directory: "/")
!6 = !{}
!7 = !{i32 7}
!8 = !{i32 6}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_gep", linkageName: "test_gep", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !16, !18, !19, !20}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)
!16 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 3, type: !17)
!17 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 4, type: !14)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 5, type: !14)
!20 = !DILocalVariable(name: "6", scope: !10, file: !5, line: 6, type: !21)
!21 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!22 = !DILocation(line: 1, column: 1, scope: !10)
!23 = !DILocation(line: 2, column: 1, scope: !10)
!24 = !DILocation(line: 3, column: 1, scope: !10)
!25 = !DILocation(line: 4, column: 1, scope: !10)
!26 = !DILocation(line: 5, column: 1, scope: !10)
!27 = !DILocation(line: 6, column: 1, scope: !10)
!28 = !DILocation(line: 7, column: 1, scope: !10)
