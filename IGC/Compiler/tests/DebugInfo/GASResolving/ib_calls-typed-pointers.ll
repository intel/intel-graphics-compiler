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
; CHECK: call void @llvm.dbg.value(metadata i8 {{.*}} [[DST_V:%[A-z0-9]*]]
; CHECK-SAME: metadata [[DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DST_LOC:![0-9]*]]
;
; CHECK: call void {{.*}}_to_private{{.*}} !dbg [[PRIV_LOC:![0-9]*]]
; CHECK: call void {{.*}} !dbg [[GLOB_LOC:![0-9]*]]
;

define spir_kernel void @test_kernel(i8* %src, i8 addrspace(1)* %dst) !dbg !10 {
entry:
  %0 = addrspacecast i8 addrspace(1)* %dst to i8 addrspace(4)*, !dbg !15
  call void @llvm.dbg.value(metadata i8 addrspace(4)* %0, metadata !13, metadata !DIExpression()), !dbg !15
  call void @__builtin_IB_memcpy_generic_to_private(i8* %src, i8 addrspace(4)* %0, i32 1, i32 8), !dbg !16
  call void @__builtin_IB_memcpy_private_to_generic(i8 addrspace(4)* %0, i8* %src, i32 1, i32 8), !dbg !17
  ret void, !dbg !18
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ib_calls.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !5, line: 1
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[PRIV_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[GLOB_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])

declare void @__builtin_IB_memcpy_generic_to_private(i8*, i8 addrspace(4)*, i32, i32)

declare void @__builtin_IB_memcpy_private_to_generic(i8 addrspace(4)*, i8*, i32, i32)

declare void @__builtin_IB_memcpy_global_to_private(i8*, i8 addrspace(1)*, i32, i32)

declare void @__builtin_IB_memcpy_private_to_global(i8 addrspace(1)*, i8*, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!llvm.dbg.cu = !{!4}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{void (i8*, i8 addrspace(1)*)* @test_kernel, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = distinct !DICompileUnit(language: DW_LANG_C, file: !5, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !6)
!5 = !DIFile(filename: "ib_calls.ll", directory: "/")
!6 = !{}
!7 = !{i32 4}
!8 = !{i32 1}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test_kernel", linkageName: "test_kernel", scope: null, file: !5, line: 1, type: !11, scopeLine: 1, unit: !4, retainedNodes: !12)
!11 = !DISubroutineType(types: !6)
!12 = !{!13}
!13 = !DILocalVariable(name: "1", scope: !10, file: !5, line: 1, type: !14)
!14 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!15 = !DILocation(line: 1, column: 1, scope: !10)
!16 = !DILocation(line: 2, column: 1, scope: !10)
!17 = !DILocation(line: 3, column: 1, scope: !10)
!18 = !DILocation(line: 4, column: 1, scope: !10)
