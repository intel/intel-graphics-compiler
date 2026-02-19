;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-gas-resolve -S < %s | FileCheck %s
; ------------------------------------------------
; GASResolve
; ------------------------------------------------
; This test checks that GASResolve pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_kernel
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value(metadata i8 {{.*}} [[SRC_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[SRC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SRC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value(metadata i8 {{.*}} [[DST_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DST_LOC:![0-9]*]]
;
; CHECK: call void @llvm.memcpy{{.*}} !dbg [[MEMCPY_LOC:![0-9]*]]
; CHECK: call void @llvm.memmove{{.*}} !dbg [[MEMMOVE_LOC:![0-9]*]]
; CHECK: call void @llvm.memset{{.*}} !dbg [[MEMSET_LOC:![0-9]*]]

define spir_kernel void @test_kernel(i8 addrspace(1)* %src, i8 addrspace(1)* %dst) !dbg !10 {
entry:
  %0 = addrspacecast i8 addrspace(1)* %src to i8 addrspace(4)*, !dbg !16
  call void @llvm.dbg.value(metadata i8 addrspace(4)* %0, metadata !13, metadata !DIExpression()), !dbg !16
  %1 = addrspacecast i8 addrspace(1)* %dst to i8 addrspace(4)*, !dbg !17
  call void @llvm.dbg.value(metadata i8 addrspace(4)* %1, metadata !15, metadata !DIExpression()), !dbg !17
  call void @llvm.memcpy.p4i8.p4i8.i32(i8 addrspace(4)* %1, i8 addrspace(4)* %0, i32 8, i1 false), !dbg !18
  call void @llvm.memmove.p4i8.p4i8.i32(i8 addrspace(4)* %0, i8 addrspace(4)* %1, i32 16, i1 false), !dbg !19
  call void @llvm.memset.p4i8.i32(i8 addrspace(4)* %1, i8 0, i32 4, i1 false), !dbg !20
  ret void, !dbg !21
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "memfunc.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[MEMCPY_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[MEMMOVE_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[MEMSET_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p4i8.p4i8.i32(i8 addrspace(4)* noalias nocapture writeonly, i8 addrspace(4)* noalias nocapture readonly, i32, i1 immarg) #0

; Function Attrs: argmemonly nounwind
declare void @llvm.memmove.p4i8.p4i8.i32(i8 addrspace(4)* nocapture, i8 addrspace(4)* nocapture readonly, i32, i1 immarg) #0

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p4i8.i32(i8 addrspace(4)* nocapture writeonly, i8, i32, i1 immarg) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { argmemonly nounwind }
attributes #1 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i8 addrspace(1)*, i8 addrspace(1)*)* @test_kernel, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "memfunc.ll", directory: "/")
!6 = !{}
!7 = !{i32 6}
!8 = !{i32 2}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13, !15}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocalVariable(name: "2", scope: !10, file: !5, line: 2, type: !14)
!16 = !DILocation(line: 1, column: 1, scope: !10)
!17 = !DILocation(line: 2, column: 1, scope: !10)
!18 = !DILocation(line: 3, column: 1, scope: !10)
!19 = !DILocation(line: 4, column: 1, scope: !10)
!20 = !DILocation(line: 5, column: 1, scope: !10)
!21 = !DILocation(line: 6, column: 1, scope: !10)
