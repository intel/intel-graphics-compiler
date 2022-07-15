;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-unsafe-opt-pass -S < %s | FileCheck %s
; ------------------------------------------------
; CustomUnsafeOptPass
; ------------------------------------------------
; This test checks that CustomUnsafeOptPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_custom{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK: void @llvm.dbg.value(metadata float [[VAL2_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: store float [[VAL2_V]]{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[VAL5_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: [[VAL7_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK: void @llvm.dbg.value(metadata float [[VAL7_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]
; CHECK: store float [[VAL7_V]]{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]

define spir_kernel void @test_custom(float %a, float %b, float %c, float* %d) !dbg !6 {
entry:
  %0 = fmul float %a, %b, !dbg !20
  call void @llvm.dbg.value(metadata float %0, metadata !9, metadata !DIExpression()), !dbg !20
  %1 = fadd float %0, %c, !dbg !21
  call void @llvm.dbg.value(metadata float %1, metadata !11, metadata !DIExpression()), !dbg !21
  %2 = fcmp oeq float %b, 0.000000e+00, !dbg !22
  call void @llvm.dbg.value(metadata i1 %2, metadata !12, metadata !DIExpression()), !dbg !22
  %3 = select i1 %2, float %c, float %1, !dbg !23
  call void @llvm.dbg.value(metadata float %3, metadata !14, metadata !DIExpression()), !dbg !23
  store float %3, float* %d, !dbg !24
  %4 = fsub float %a, %b, !dbg !25
  call void @llvm.dbg.value(metadata float %4, metadata !15, metadata !DIExpression()), !dbg !25
  %5 = fmul float %4, %c, !dbg !26
  call void @llvm.dbg.value(metadata float %5, metadata !16, metadata !DIExpression()), !dbg !26
  %6 = fadd float %5, %b, !dbg !27
  call void @llvm.dbg.value(metadata float %6, metadata !17, metadata !DIExpression()), !dbg !27
  %7 = fcmp oeq float %c, 1.000000e+00, !dbg !28
  call void @llvm.dbg.value(metadata i1 %7, metadata !18, metadata !DIExpression()), !dbg !28
  %8 = select i1 %7, float %a, float %6, !dbg !29
  call void @llvm.dbg.value(metadata float %8, metadata !19, metadata !DIExpression()), !dbg !29
  store float %8, float* %d, !dbg !30
  ret void, !dbg !31
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "select.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare float @llvm.floor.f32(float) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "select.ll", directory: "/")
!2 = !{}
!3 = !{i32 12}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15, !16, !17, !18, !19}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 7, type: !10)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 8, type: !10)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 9, type: !13)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 10, type: !10)
!20 = !DILocation(line: 1, column: 1, scope: !6)
!21 = !DILocation(line: 2, column: 1, scope: !6)
!22 = !DILocation(line: 3, column: 1, scope: !6)
!23 = !DILocation(line: 4, column: 1, scope: !6)
!24 = !DILocation(line: 5, column: 1, scope: !6)
!25 = !DILocation(line: 6, column: 1, scope: !6)
!26 = !DILocation(line: 7, column: 1, scope: !6)
!27 = !DILocation(line: 8, column: 1, scope: !6)
!28 = !DILocation(line: 9, column: 1, scope: !6)
!29 = !DILocation(line: 10, column: 1, scope: !6)
!30 = !DILocation(line: 11, column: 1, scope: !6)
!31 = !DILocation(line: 12, column: 1, scope: !6)
