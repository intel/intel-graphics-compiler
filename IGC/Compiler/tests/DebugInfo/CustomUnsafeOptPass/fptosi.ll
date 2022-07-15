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
; CHECK: [[VAL3_V:%[A-z0-9]*]] = fadd fast float {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9]*]] = fadd fast float {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL7_V:%[A-z0-9]*]] = fadd fast float {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK: void @llvm.dbg.value(metadata float [[VAL7_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC:![0-9]*]]
; CHECK: [[VAL10_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL10_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL10_V]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC]]

define spir_kernel void @test_custom(i1 %a, i1 %aa, float %b, float %c, i32* %d) !dbg !6 {
entry:
  %a1 = and i1 %a, %aa, !dbg !23
  call void @llvm.dbg.value(metadata i1 %a1, metadata !9, metadata !DIExpression()), !dbg !23
  %a2 = xor i1 %a, %aa, !dbg !24
  call void @llvm.dbg.value(metadata i1 %a2, metadata !11, metadata !DIExpression()), !dbg !24
  %0 = select i1 %a, float 1.000000e+00, float 0.000000e+00, !dbg !25
  call void @llvm.dbg.value(metadata float %0, metadata !12, metadata !DIExpression()), !dbg !25
  %1 = select i1 %aa, float 1.000000e+00, float 0.000000e+00, !dbg !26
  call void @llvm.dbg.value(metadata float %1, metadata !14, metadata !DIExpression()), !dbg !26
  %2 = fadd fast float %1, %0, !dbg !27
  call void @llvm.dbg.value(metadata float %2, metadata !15, metadata !DIExpression()), !dbg !27
  %3 = select i1 %a1, float 1.000000e+00, float 0.000000e+00, !dbg !28
  call void @llvm.dbg.value(metadata float %3, metadata !16, metadata !DIExpression()), !dbg !28
  %4 = fadd fast float %3, %2, !dbg !29
  call void @llvm.dbg.value(metadata float %4, metadata !17, metadata !DIExpression()), !dbg !29
  %5 = select i1 %a2, float 1.000000e+00, float 0.000000e+00, !dbg !30
  call void @llvm.dbg.value(metadata float %5, metadata !18, metadata !DIExpression()), !dbg !30
  %6 = fadd fast float %5, %4, !dbg !31
  call void @llvm.dbg.value(metadata float %6, metadata !19, metadata !DIExpression()), !dbg !31
  %7 = fadd fast float %6, 5.000000e-01, !dbg !32
  call void @llvm.dbg.value(metadata float %7, metadata !20, metadata !DIExpression()), !dbg !32
  %8 = call fast float @llvm.floor.f32(float %7), !dbg !33
  call void @llvm.dbg.value(metadata float %8, metadata !21, metadata !DIExpression()), !dbg !33
  %9 = fptosi float %8 to i32, !dbg !34
  call void @llvm.dbg.value(metadata i32 %9, metadata !22, metadata !DIExpression()), !dbg !34
  store i32 %9, i32* %d, !dbg !35
  ret void, !dbg !36
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "fptosi.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare float @llvm.floor.f32(float) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "fptosi.ll", directory: "/")
!2 = !{}
!3 = !{i32 14}
!4 = !{i32 12}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !14, !15, !16, !17, !18, !19, !20, !21, !22}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !13)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !13)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !13)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !13)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !13)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !13)
!20 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !13)
!21 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 11, type: !13)
!22 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 12, type: !13)
!23 = !DILocation(line: 1, column: 1, scope: !6)
!24 = !DILocation(line: 2, column: 1, scope: !6)
!25 = !DILocation(line: 3, column: 1, scope: !6)
!26 = !DILocation(line: 4, column: 1, scope: !6)
!27 = !DILocation(line: 5, column: 1, scope: !6)
!28 = !DILocation(line: 6, column: 1, scope: !6)
!29 = !DILocation(line: 7, column: 1, scope: !6)
!30 = !DILocation(line: 8, column: 1, scope: !6)
!31 = !DILocation(line: 9, column: 1, scope: !6)
!32 = !DILocation(line: 10, column: 1, scope: !6)
!33 = !DILocation(line: 11, column: 1, scope: !6)
!34 = !DILocation(line: 12, column: 1, scope: !6)
!35 = !DILocation(line: 13, column: 1, scope: !6)
!36 = !DILocation(line: 14, column: 1, scope: !6)
