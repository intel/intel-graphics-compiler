;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-scalarize -S < %s | FileCheck %s
; ------------------------------------------------
; ScalarizeFunction : binary operands
; ------------------------------------------------
; This test checks that ScalarizeFunction pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; Check IR:
;
; CHECK: alloca {{.*}}, !dbg [[ALLOC_LOC:![0-9]*]]
; CHECK: dbg.declare({{.*}}, metadata [[R_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOC_LOC]]
; CHECK-DAG: dbg.value(metadata <2 x i32> [[ADD_V:%[a-z0-9\.]*]], metadata [[ADD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ADD_LOC:![0-9]*]]
; CHECK-DAG: [[ADD_V]] = {{.*}}, !dbg [[ADD_LOC]]
; CHECK-DAG: store <2 x i32> [[ADD_V]], {{.*}}, !dbg [[STORE_LOC:![0-9]*]]
;

define spir_kernel void @test_binary(<2 x i32> %src1, <2 x i32> %src2) !dbg !6 {
  %1 = alloca <2 x i32>, align 4, !dbg !12
  call void @llvm.dbg.declare(metadata <2 x i32>* %1, metadata !9, metadata !DIExpression()), !dbg !12
  %2 = add <2 x i32> %src1, %src2, !dbg !13
  call void @llvm.dbg.value(metadata <2 x i32> %2, metadata !11, metadata !DIExpression()), !dbg !13
  store <2 x i32> %2, <2 x i32>* %1, !dbg !14
  ret void, !dbg !15
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ScalarizeFunction/binary.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_binary", linkageName: "test_binary", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ALLOC_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[R_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[ADD_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[ADD_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "ScalarizeFunction/binary.ll", directory: "/")
!2 = !{}
!3 = !{i32 4}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_binary", linkageName: "test_binary", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocation(line: 1, column: 1, scope: !6)
!13 = !DILocation(line: 2, column: 1, scope: !6)
!14 = !DILocation(line: 3, column: 1, scope: !6)
!15 = !DILocation(line: 4, column: 1, scope: !6)
