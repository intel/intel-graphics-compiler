;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-vectorpreprocess -S < %s | FileCheck %s
; ------------------------------------------------
; VectorPreProcess : load aligned
; ------------------------------------------------
; This test checks that VectorPreProcess pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK:  spir_kernel void @test_vecpre
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ST_V:%[A-z0-9]*]] = alloca
; CHECK: @llvm.dbg.declare(metadata {{.*}} [[ST_V]], metadata [[ST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ST_LOC:![0-9]*]]
; CHECK: [[LOAD1_V:%[A-z0-9*]]] = load {{.*}}, !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK-DAG: [[EXTR1_V:%[A-z0-9]*]] = extractelement {{.*}} 0, !dbg [[EXTR1_LOC:![0-9]*]]
; CHECK-DAG: [[EXTR2_V:%[A-z0-9]*]] = extractelement {{.*}} 1, !dbg [[EXTR2_LOC:![0-9]*]]
; CHECK-DAG: [[EXTR3_V:%[A-z0-9]*]] = extractelement {{.*}} 2, !dbg [[EXTR3_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata {{.*}}, metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata {{.*}} [[EXTR1_V]], metadata [[EXTR1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR1_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata {{.*}} [[EXTR2_V]], metadata [[EXTR2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR2_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata {{.*}} [[EXTR3_V]], metadata [[EXTR3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXTR3_LOC]]
; CHECK-DAG: [[CALL1_V:%[A-z0-9*]]] = call float {{.*}}, !dbg [[CALL1_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata <4 x float> {{.*}}, metadata [[CALL0_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL0_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata float [[CALL1_V]], metadata [[CALL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL1_LOC]]
; CHECK-DAG: [[CALL3_V:%[A-z0-9*]]] = call float {{.*}}, !dbg [[CALL3_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata <4 x float> {{.*}}, metadata [[CALL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL2_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata float [[CALL3_V]], metadata [[CALL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL3_LOC]]

define spir_kernel void @test_vecpre(<4 x float> addrspace(1)* %a) !dbg !6 {
  %st = alloca float, align 4, !dbg !24
  call void @llvm.dbg.declare(metadata float* %st, metadata !9, metadata !DIExpression()), !dbg !24
  %1 = load <4 x float>, <4 x float> addrspace(1)* %a, align 4, !dbg !25
  call void @llvm.dbg.value(metadata <4 x float> %1, metadata !11, metadata !DIExpression()), !dbg !25
  %2 = extractelement <4 x float> %1, i32 0, !dbg !26
  call void @llvm.dbg.value(metadata float %2, metadata !13, metadata !DIExpression()), !dbg !26
  %3 = extractelement <4 x float> %1, i32 1, !dbg !27
  call void @llvm.dbg.value(metadata float %3, metadata !15, metadata !DIExpression()), !dbg !27
  %4 = extractelement <4 x float> %1, i32 2, !dbg !28
  call void @llvm.dbg.value(metadata float %4, metadata !16, metadata !DIExpression()), !dbg !28
  %5 = call <4 x float> @llvm.genx.GenISA.ldrawvector_indexed.p1v4f32(<4 x float> addrspace(1)* %a, i32 0, i32 4, i1 false), !dbg !29
  call void @llvm.dbg.value(metadata <4 x float> %5, metadata !17, metadata !DIExpression()), !dbg !29
  %6 = extractelement <4 x float> %5, i32 0, !dbg !30
  call void @llvm.dbg.value(metadata float %6, metadata !18, metadata !DIExpression()), !dbg !30
  %7 = call <4 x float> @llvm.genx.GenISA.ldrawvector_indexed.p1v4f32(<4 x float> addrspace(1)* %a, i32 100, i32 4, i1 false), !dbg !31
  call void @llvm.dbg.value(metadata <4 x float> %7, metadata !19, metadata !DIExpression()), !dbg !31
  %8 = extractelement <4 x float> %7, i32 1, !dbg !32
  call void @llvm.dbg.value(metadata float %8, metadata !20, metadata !DIExpression()), !dbg !32
  %9 = fadd float %2, %3, !dbg !33
  call void @llvm.dbg.value(metadata float %9, metadata !21, metadata !DIExpression()), !dbg !33
  %10 = fadd float %9, %6, !dbg !34
  call void @llvm.dbg.value(metadata float %10, metadata !22, metadata !DIExpression()), !dbg !34
  %11 = fadd float %10, %8, !dbg !35
  call void @llvm.dbg.value(metadata float %11, metadata !23, metadata !DIExpression()), !dbg !35
  store float %11, float* %st, !dbg !36
  ret void, !dbg !37
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "load_simple.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vecpre", linkageName: "test_vecpre", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR1_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[EXTR1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR2_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[EXTR2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXTR3_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[EXTR3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL0_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[CALL0_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL1_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[CALL1_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL2_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[CALL2_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL3_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[CALL3_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])

declare <4 x float> @llvm.genx.GenISA.ldrawvector_indexed.p1v4f32(<4 x float> addrspace(1)*, i32, i32, i1)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "load_simple.ll", directory: "/")
!2 = !{}
!3 = !{i32 14}
!4 = !{i32 12}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_vecpre", linkageName: "test_vecpre", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !15, !16, !17, !18, !19, !20, !21, !22, !23}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !14)
!16 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !14)
!17 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !12)
!18 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !14)
!19 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !12)
!20 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !14)
!21 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !14)
!22 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 11, type: !14)
!23 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 12, type: !14)
!24 = !DILocation(line: 1, column: 1, scope: !6)
!25 = !DILocation(line: 2, column: 1, scope: !6)
!26 = !DILocation(line: 3, column: 1, scope: !6)
!27 = !DILocation(line: 4, column: 1, scope: !6)
!28 = !DILocation(line: 5, column: 1, scope: !6)
!29 = !DILocation(line: 6, column: 1, scope: !6)
!30 = !DILocation(line: 7, column: 1, scope: !6)
!31 = !DILocation(line: 8, column: 1, scope: !6)
!32 = !DILocation(line: 9, column: 1, scope: !6)
!33 = !DILocation(line: 10, column: 1, scope: !6)
!34 = !DILocation(line: 11, column: 1, scope: !6)
!35 = !DILocation(line: 12, column: 1, scope: !6)
!36 = !DILocation(line: 13, column: 1, scope: !6)
!37 = !DILocation(line: 14, column: 1, scope: !6)
