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
; this value is unsalvageble
; CHECK: void @llvm.dbg.value({{.*}}
; this value is unsalvageble
; CHECK: void @llvm.dbg.value({{.*}}
; CHECK-DAG: void @llvm.dbg.value(metadata float [[VAL4_V:%[A-z0-9]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]

define spir_kernel void @test_custom(float %a, float addrspace(65549)* %b) !dbg !9 {
entry:
  %0 = load float, float addrspace(65549)* %b, !dbg !17
  call void @llvm.dbg.value(metadata float %0, metadata !12, metadata !DIExpression()), !dbg !17
  %1 = fmul float %a, %0, !dbg !18
  call void @llvm.dbg.value(metadata float %1, metadata !14, metadata !DIExpression()), !dbg !18
  %2 = fmul float %a, %0, !dbg !19
  call void @llvm.dbg.value(metadata float %2, metadata !15, metadata !DIExpression()), !dbg !19
  %3 = fadd float %1, %2, !dbg !20
  call void @llvm.dbg.value(metadata float %3, metadata !16, metadata !DIExpression()), !dbg !20
  store float %3, float addrspace(65549)* %b, !dbg !21
  ret void, !dbg !22
}


; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "exchange-cb.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

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
!4 = !DIFile(filename: "exchange-cb.ll", directory: "/")
!5 = !{}
!6 = !{i32 6}
!7 = !{i32 4}
!8 = !{i32 2, !"Debug Info Version", i32 3}
!9 = distinct !DISubprogram(name: "test_custom", linkageName: "test_custom", scope: null, file: !4, line: 1, type: !10, scopeLine: 1, unit: !3, retainedNodes: !11)
!10 = !DISubroutineType(types: !5)
!11 = !{!12, !14, !15, !16}
!12 = !DILocalVariable(name: "1", scope: !9, file: !4, line: 1, type: !13)
!13 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!14 = !DILocalVariable(name: "2", scope: !9, file: !4, line: 2, type: !13)
!15 = !DILocalVariable(name: "3", scope: !9, file: !4, line: 3, type: !13)
!16 = !DILocalVariable(name: "4", scope: !9, file: !4, line: 4, type: !13)
!17 = !DILocation(line: 1, column: 1, scope: !9)
!18 = !DILocation(line: 2, column: 1, scope: !9)
!19 = !DILocation(line: 3, column: 1, scope: !9)
!20 = !DILocation(line: 4, column: 1, scope: !9)
!21 = !DILocation(line: 5, column: 1, scope: !9)
!22 = !DILocation(line: 6, column: 1, scope: !9)
