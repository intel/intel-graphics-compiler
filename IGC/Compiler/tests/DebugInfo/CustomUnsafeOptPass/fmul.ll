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
; CHECK: void @llvm.dbg.value(metadata float [[VAL1_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL3_V:%[A-z0-9]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK: store float [[VAL3_V]]{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; this value is unsalvageble
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL5_V:%[A-z0-9]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK: store float [[VAL5_V]]{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]
; this value is unsalvageble
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL7_V:%[A-z0-9]*]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V]] = {{.*}}, !dbg [[VAL7_LOC]]
; CHECK: store float [[VAL7_V]]{{.*}}, !dbg [[STORE3_LOC:![0-9]*]]

define spir_kernel void @test_custom(float %a, float addrspace(65549)* %b) !dbg !9 {
entry:
  %0 = load float, float addrspace(65549)* %b, !dbg !20
  call void @llvm.dbg.value(metadata float %0, metadata !12, metadata !DIExpression()), !dbg !20
  %1 = fmul float %0, 1.000000e+00, !dbg !21
  call void @llvm.dbg.value(metadata float %1, metadata !14, metadata !DIExpression()), !dbg !21
  %2 = fmul float %1, -1.0, !dbg !22
  call void @llvm.dbg.value(metadata float %2, metadata !15, metadata !DIExpression()), !dbg !22
  store float %2, float addrspace(65549)* %b, !dbg !23
  %3 = fsub float 1.000000e+00, %a, !dbg !24
  call void @llvm.dbg.value(metadata float %3, metadata !16, metadata !DIExpression()), !dbg !24
  %4 = fmul float %3, %0, !dbg !25
  call void @llvm.dbg.value(metadata float %4, metadata !17, metadata !DIExpression()), !dbg !25
  store float %4, float addrspace(65549)* %b, !dbg !26
  %5 = fadd float %0, 1.350000e+01, !dbg !27
  call void @llvm.dbg.value(metadata float %5, metadata !18, metadata !DIExpression()), !dbg !27
  %6 = fmul float %5, 2.130000e+02, !dbg !28
  call void @llvm.dbg.value(metadata float %6, metadata !19, metadata !DIExpression()), !dbg !28
  store float %6, float addrspace(65549)* %b, !dbg !29
  ret void, !dbg !30
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "fmul.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE3_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!llvm.dbg.cu = !{!3}
!llvm.debugify = !{!6, !7}
!llvm.module.flags = !{!8}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
!3 = distinct !DICompileUnit(language: DW_LANG_C, file: !4, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "fmul.ll", directory: "/")
!5 = !{}
!6 = !{i32 11}
!7 = !{i32 7}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !16, !17, !18, !19}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !13)
!16 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 5, type: !13)
!17 = !DILocalVariable(name: "5", scope: !9, file: !4, line: 6, type: !13)
!18 = !DILocalVariable(name: "6", scope: !9, file: !4, line: 8, type: !13)
!19 = !DILocalVariable(name: "7", scope: !9, file: !4, line: 9, type: !13)
!20 = !DILocation(line: 1, column: 1, scope: !9)
!21 = !DILocation(line: 2, column: 1, scope: !9)
!22 = !DILocation(line: 3, column: 1, scope: !9)
!23 = !DILocation(line: 4, column: 1, scope: !9)
!24 = !DILocation(line: 5, column: 1, scope: !9)
!25 = !DILocation(line: 6, column: 1, scope: !9)
!26 = !DILocation(line: 7, column: 1, scope: !9)
!27 = !DILocation(line: 8, column: 1, scope: !9)
!28 = !DILocation(line: 9, column: 1, scope: !9)
!29 = !DILocation(line: 10, column: 1, scope: !9)
!30 = !DILocation(line: 11, column: 1, scope: !9)
