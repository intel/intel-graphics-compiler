;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-promote-stateless-to-bindless -S < %s | FileCheck %s
; ------------------------------------------------
; PromoteStatelessToBindless : load and store part
; ------------------------------------------------
; This test checks that PromoteStatelessToBindless pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------


; CHECK: void @test_promote
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK-DAG: @llvm.dbg.value(metadata i32 [[LOAD_V:%[A-z0-9]*]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD_V]] = {{.*}}, !dbg [[LOAD_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata float addrspace(1)* [[ACAST_V:%[A-z0-9]*]], metadata [[ACAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST_LOC:![0-9]*]]
; CHECK-DAG: [[ACAST_V]] = {{.*}}, !dbg [[ACAST_LOC]]
;
; CHECK-DAG: @llvm.dbg.value(metadata i32 addrspace(1)* [[BCAST_V:%[A-z0-9]*]], metadata [[BCAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST_LOC:![0-9]*]]
; CHECK-DAG: [[BCAST_V]] = {{.*}}, !dbg [[BCAST_LOC]]

; CHECK: !dbg [[STORE_LOC:![0-9]*]]
; CHECK: ret

define spir_kernel void @test_promote(i32 addrspace(1)* %src, float addrspace(2)* %dst) !dbg !10 {
  %1 = load i32, i32 addrspace(1)* %src, !dbg !18
  call void @llvm.dbg.value(metadata i32 %1, metadata !13, metadata !DIExpression()), !dbg !18
  %2 = addrspacecast float addrspace(2)* %dst to float addrspace(1)*, !dbg !19
  call void @llvm.dbg.value(metadata float addrspace(1)* %2, metadata !15, metadata !DIExpression()), !dbg !19
  %3 = bitcast float addrspace(1)* %2 to i32 addrspace(1)*, !dbg !20
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %3, metadata !17, metadata !DIExpression()), !dbg !20
  store i32 %1, i32 addrspace(1)* %3, !dbg !21
  ret void, !dbg !22
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "loadstore.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ACAST_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[ACAST_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCAST_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[BCAST_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i32 addrspace(1)*, float addrspace(2)*)* @test_promote, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "loadstore.ll", directory: "/")
!6 = !{}
!7 = !{i32 5}
!8 = !{i32 3}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15, !17}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !16)
!16 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!17 = !DILocalVariable(name: "3", scope: !10, file: !5, line: 3, type: !16)
!18 = !DILocation(line: 1, column: 1, scope: !10)
!19 = !DILocation(line: 2, column: 1, scope: !10)
!20 = !DILocation(line: 3, column: 1, scope: !10)
!21 = !DILocation(line: 4, column: 1, scope: !10)
!22 = !DILocation(line: 5, column: 1, scope: !10)
