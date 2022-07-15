;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-mcs-optimization -S < %s | FileCheck %s
; ------------------------------------------------
; MCSOptimization
; ------------------------------------------------
; This test checks that MCSOptimization pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x float> [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]

; CHECK-DAG: void @llvm.dbg.value(metadata <4 x float> [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata <4 x float> [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]


define void @test(<4 x float> addrspace(2)* %s1, <4 x float>* %dst) !dbg !10 {
  %1 = call <4 x i32> @llvm.genx.GenISA.ldmcsptr.v4i32.i32.p2v4f32(i32 1, i32 2, i32 3, i32 4, <4 x float> addrspace(2)* %s1, i32 0, i32 0, i32 0), !dbg !20
  call void @llvm.dbg.value(metadata <4 x i32> %1, metadata !13, metadata !DIExpression()), !dbg !20
  %2 = extractelement <4 x i32> %1, i32 0, !dbg !21
  call void @llvm.dbg.value(metadata i32 %2, metadata !15, metadata !DIExpression()), !dbg !21
  %3 = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32 2, i32 %2, i32 4, i32 5, i32 1, i32 2, i32 3, <4 x float> addrspace(2)* %s1, i32 0, i32 0, i32 0), !dbg !22
  call void @llvm.dbg.value(metadata <4 x float> %3, metadata !17, metadata !DIExpression()), !dbg !22
  %4 = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32 2, i32 %2, i32 4, i32 5, i32 1, i32 2, i32 3, <4 x float> addrspace(2)* %s1, i32 0, i32 0, i32 0), !dbg !23
  call void @llvm.dbg.value(metadata <4 x float> %4, metadata !18, metadata !DIExpression()), !dbg !23
  %5 = fadd <4 x float> %3, %4, !dbg !24
  call void @llvm.dbg.value(metadata <4 x float> %5, metadata !19, metadata !DIExpression()), !dbg !24
  store <4 x float> %5, <4 x float>* %dst, !dbg !25
  ret void, !dbg !26
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "MCSOptimization.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

declare <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32, i32, i32, i32, i32, i32, i32, <4 x float> addrspace(2)*, i32, i32, i32)

declare <4 x i32> @llvm.genx.GenISA.ldmcsptr.v4i32.i32.p2v4f32(i32, i32, i32, i32, <4 x float> addrspace(2)*, i32, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{}
!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{!"ModuleMD", !1}
!1 = !{!"m_ShaderResourceViewMcsMask", !2, !3}
!2 = !{!"m_ShaderResourceViewMcsMaskVec[0]", i64 4}
!3 = !{!"m_ShaderResourceViewMcsMaskVec[1]", i64 4}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "MCSOptimization.ll", directory: "/")
!6 = !{}
!7 = !{i32 7}
!8 = !{i32 5}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !17, !18, !19}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !16)
!16 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 3, type: !14)
!18 = !DILocalVariable(name: "4", scope: !10, file: !5, line: 4, type: !14)
!19 = !DILocalVariable(name: "5", scope: !10, file: !5, line: 5, type: !14)
!20 = !DILocation(line: 1, column: 1, scope: !10)
!21 = !DILocation(line: 2, column: 1, scope: !10)
!22 = !DILocation(line: 3, column: 1, scope: !10)
!23 = !DILocation(line: 4, column: 1, scope: !10)
!24 = !DILocation(line: 5, column: 1, scope: !10)
!25 = !DILocation(line: 6, column: 1, scope: !10)
!26 = !DILocation(line: 7, column: 1, scope: !10)
