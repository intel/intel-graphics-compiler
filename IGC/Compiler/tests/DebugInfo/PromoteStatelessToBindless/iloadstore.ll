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
; PromoteStatelessToBindless : load and store intrinsics part
; ------------------------------------------------
; This test checks that PromoteStatelessToBindless pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: void @test_promote
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK-DAG: @llvm.dbg.value(metadata <2 x float> [[LOAD_V:%[A-z0-9]*]], metadata [[LOAD_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD_LOC:![0-9]*]]
; CHECK-DAG: [[LOAD_V]] = {{.*}}, !dbg [[LOAD_LOC]]
;
; CHECK: !dbg [[STORE_LOC:![0-9]*]]
; CHECK: ret

define spir_kernel void @test_promote(i8 addrspace(1)* %src, i8 addrspace(2)* %dst) !dbg !10 {
  %1 = call <2 x float> @llvm.genx.GenISA.simdBlockRead.v2f32.p1(i8 addrspace(1)* %src), !dbg !15
  call void @llvm.dbg.value(metadata <2 x float> %1, metadata !13, metadata !DIExpression()), !dbg !15
  call void @llvm.genx.GenISA.simdBlockWrite.v2f32.p2(i8 addrspace(2)* %dst, <2 x float> %1), !dbg !16
  ret void, !dbg !17
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "iloadstore.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[LOAD_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])

declare <2 x float> @llvm.genx.GenISA.simdBlockRead.v2f32.p1(i8 addrspace(1)*)

declare void @llvm.genx.GenISA.simdBlockWrite.v2f32.p2(i8 addrspace(2)*, <2 x float>)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i8 addrspace(1)*, i8 addrspace(2)*)* @test_promote, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "iloadstore.ll", directory: "/")
!6 = !{}
!7 = !{i32 3}
!8 = !{i32 1}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_promote", linkageName: "test_promote", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocation(line: 1, column: 1, scope: !10)
!16 = !DILocation(line: 2, column: 1, scope: !10)
!17 = !DILocation(line: 3, column: 1, scope: !10)
