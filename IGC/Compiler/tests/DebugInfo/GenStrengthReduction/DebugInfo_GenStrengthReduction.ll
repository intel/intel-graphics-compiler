;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -GenStrengthReduction -S < %s | FileCheck %s
; ------------------------------------------------
; GenStrengthReduction
; ------------------------------------------------
; This test checks that GenStrengthReduction pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.

; CHECK: define spir_kernel void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float %a, metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: store float %a, {{.*}}, !dbg [[STR1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL2_V:[0.e+]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: store float [[VAL2_V]], {{.*}}, !dbg [[STR2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL3_V:[0.e+]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: store float [[VAL3_V]], {{.*}}, !dbg [[STR3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float %a, metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: store float %a, {{.*}}, !dbg [[STR4_LOC:![0-9]*]]
; CHECK-DAG: store float [[VAL6_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR5_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]
; CHECK-DAG: store float [[VAL7_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR6_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK-DAG: store float [[VAL8_V:%[A-z0-9]*]], {{.*}}, !dbg [[STR7_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL8_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V]] = {{.*}}, !dbg [[VAL8_LOC]]

define spir_kernel void @test(float %a, float* %b, i1 %c, float %d) !dbg !6 {
  %1 = select i1 %c, float %a, float %a, !dbg !18
  call void @llvm.dbg.value(metadata float %1, metadata !9, metadata !DIExpression()), !dbg !18
  store float %1, float* %b, !dbg !19
  %2 = fdiv nnan ninf nsz float 0.000000e+00, %a, !dbg !20
  call void @llvm.dbg.value(metadata float %2, metadata !11, metadata !DIExpression()), !dbg !20
  store float %2, float* %b, !dbg !21
  %3 = fmul nnan ninf nsz float %a, 0.000000e+00, !dbg !22
  call void @llvm.dbg.value(metadata float %3, metadata !12, metadata !DIExpression()), !dbg !22
  store float %3, float* %b, !dbg !23
  %4 = fadd nnan ninf nsz float %a, 0.000000e+00, !dbg !24
  call void @llvm.dbg.value(metadata float %4, metadata !13, metadata !DIExpression()), !dbg !24
  store float %4, float* %b, !dbg !25
  %5 = load float, float* %b, !dbg !26
  call void @llvm.dbg.value(metadata float %5, metadata !14, metadata !DIExpression()), !dbg !26
  %6 = fdiv arcp float %a, %5, !dbg !27
  call void @llvm.dbg.value(metadata float %6, metadata !15, metadata !DIExpression()), !dbg !27
  store float %6, float* %b, !dbg !28
  %7 = fdiv arcp float %d, %5, !dbg !29
  call void @llvm.dbg.value(metadata float %7, metadata !16, metadata !DIExpression()), !dbg !29
  store float %7, float* %b, !dbg !30
  %8 = fdiv arcp float %d, %a, !dbg !31
  call void @llvm.dbg.value(metadata float %8, metadata !17, metadata !DIExpression()), !dbg !31
  store float %8, float* %b, !dbg !32
  ret void, !dbg !33
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenStrengthReduction.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR3_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR4_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR5_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR6_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR7_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenStrengthReduction.ll", directory: "/")
!2 = !{}
!3 = !{i32 16}
!4 = !{i32 8}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15, !16, !17}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 5, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 7, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 9, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 10, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 12, type: !10)
!17 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 14, type: !10)
!18 = !DILocation(line: 1, column: 1, scope: !6)
!19 = !DILocation(line: 2, column: 1, scope: !6)
!20 = !DILocation(line: 3, column: 1, scope: !6)
!21 = !DILocation(line: 4, column: 1, scope: !6)
!22 = !DILocation(line: 5, column: 1, scope: !6)
!23 = !DILocation(line: 6, column: 1, scope: !6)
!24 = !DILocation(line: 7, column: 1, scope: !6)
!25 = !DILocation(line: 8, column: 1, scope: !6)
!26 = !DILocation(line: 9, column: 1, scope: !6)
!27 = !DILocation(line: 10, column: 1, scope: !6)
!28 = !DILocation(line: 11, column: 1, scope: !6)
!29 = !DILocation(line: 12, column: 1, scope: !6)
!30 = !DILocation(line: 13, column: 1, scope: !6)
!31 = !DILocation(line: 14, column: 1, scope: !6)
!32 = !DILocation(line: 15, column: 1, scope: !6)
!33 = !DILocation(line: 16, column: 1, scope: !6)
