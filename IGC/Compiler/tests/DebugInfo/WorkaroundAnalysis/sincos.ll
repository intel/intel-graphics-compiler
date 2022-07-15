;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-workaround -S < %s | FileCheck %s
; ------------------------------------------------
; WorkaroundAnalysis : Sin/Cos lowering part
; ------------------------------------------------
; This test checks that WorkaroundAnalysis pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK:  spir_kernel void @test_wa
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ST_V:%[A-z0-9]*]] = alloca
; CHECK: @llvm.dbg.declare(metadata {{.*}} [[ST_V]], metadata [[ST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ST_LOC:![0-9]*]]
; CHECK-DAG: @llvm.dbg.value(metadata {{.*}} [[SIN_V:%[A-z0-9]*]], metadata [[SIN_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SIN_LOC:![0-9]*]]
; CHECK-DAG: [[SIN_V]] = {{.*}}, !dbg [[SIN_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata {{.*}} [[COS_V:%[A-z0-9]*]], metadata [[COS_MD:![0-9]*]], metadata !DIExpression()), !dbg [[COS_LOC:![0-9]*]]
; CHECK-DAG: [[COS_V]] = {{.*}}, !dbg [[COS_LOC]]

define spir_kernel void @test_wa(float %src) !dbg !8 {
  %st = alloca float, align 4, !dbg !16
  call void @llvm.dbg.declare(metadata float* %st, metadata !11, metadata !DIExpression()), !dbg !16
  %1 = call float @llvm.sin.f32(float %src), !dbg !17
  call void @llvm.dbg.value(metadata float %1, metadata !13, metadata !DIExpression()), !dbg !17
  %2 = call float @llvm.cos.f32(float %src), !dbg !18
  call void @llvm.dbg.value(metadata float %2, metadata !15, metadata !DIExpression()), !dbg !18
  store float %1, float* %st, !dbg !19
  ret void, !dbg !20
}
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "sincos.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_wa", linkageName: "test_wa", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[SIN_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[SIN_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[COS_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[COS_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare float @llvm.sin.f32(float) #0

; Function Attrs: nounwind readnone speculatable
declare float @llvm.cos.f32(float) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!2}
!llvm.debugify = !{!5, !6}
!llvm.module.flags = !{!7}

!0 = !{!"ModuleMD", !1}
!1 = !{!"enableRangeReduce", i1 true}
!2 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "sincos.ll", directory: "/")
!4 = !{}
!5 = !{i32 5}
!6 = !{i32 3}
!7 = !{i32 2, !"Debug Info Version", i32 3}
!8 = distinct !DISubprogram(name: "test_wa", linkageName: "test_wa", scope: null, file: !3, line: 1, type: !9, scopeLine: 1, unit: !2, retainedNodes: !10)
!9 = !DISubroutineType(types: !4)
!10 = !{!11, !13, !15}
!11 = !DILocalVariable(name: "1", scope: !8, file: !3, line: 1, type: !12)
!12 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "2", scope: !8, file: !3, line: 2, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "3", scope: !8, file: !3, line: 3, type: !14)
!16 = !DILocation(line: 1, column: 1, scope: !8)
!17 = !DILocation(line: 2, column: 1, scope: !8)
!18 = !DILocation(line: 3, column: 1, scope: !8)
!19 = !DILocation(line: 4, column: 1, scope: !8)
!20 = !DILocation(line: 5, column: 1, scope: !8)
