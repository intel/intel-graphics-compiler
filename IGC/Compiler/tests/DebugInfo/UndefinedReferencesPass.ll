;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -undefined-references -S < %s | FileCheck %s
; ------------------------------------------------
; UndefinedReferencesPass
; ------------------------------------------------
; This test checks that UndefinedReferencesPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; static __constant int a[2] = {0, 1};
; __global int b;
; static __global int c;
; static __constant int* d = a;
;
; __kernel void test_(global int *dst)
; {
;     int aa = a[1];
;     c = *d;
; }
;
; ------------------------------------------------

; Check that internal gvars with at least 1 use are preserved and their MD is preserved
; CHECK: @a = {{.*}}, !dbg [[A_MDE:![0-9]*]]
; CHECK: @c = {{.*}}, !dbg [[C_MDE:![0-9]*]]
; CHECK-NOT: @e
; CHECK: @b = {{.*}}, !dbg [[B_MDE:![0-9]*]]
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "UndefinedReferencesPass.ll", directory: "dir")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DICompileUnit(language: DW_LANG_C99, file: {{.*}}, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: {{.*}}, globals: [[GLOBAL_MD:![0-9]*]])
; CHECK-DAG:[[A_MDE]] = !DIGlobalVariableExpression(var: [[A_MD:![0-9]*]], expr: !DIExpression())
; CHECK-DAG:[[A_MD]] = distinct !DIGlobalVariable(name: "a", scope: [[SCOPE]], file: [[FILE]], line: 1, type: {{.*}}, isLocal: true, isDefinition: true)
; CHECK-DAG:[[B_MDE]] = !DIGlobalVariableExpression(var: [[B_MD:![0-9]*]], expr: !DIExpression())
; CHECK-DAG:[[B_MD]] = distinct !DIGlobalVariable(name: "b", scope: [[SCOPE]], file: [[FILE]], line: 2, type: {{.*}}, isLocal: true, isDefinition: true)
; CHECK-DAG:[[C_MDE]] = !DIGlobalVariableExpression(var: [[C_MD:![0-9]*]], expr: !DIExpression())
; CHECK-DAG:[[C_MD]] = distinct !DIGlobalVariable(name: "c", scope: [[SCOPE]], file: [[FILE]], line: 3, type: {{.*}}, isLocal: true, isDefinition: true)
; CHECK-DAG:[[E_MD:![0-9]*]] = distinct !DIGlobalVariable(name: "e", scope: [[SCOPE]], file: [[FILE]], line: 4, type: {{.*}}, isLocal: true, isDefinition: true)
; CHECK-DAG:[[E_MDE:![0-9]*]] = !DIGlobalVariableExpression(var: [[E_MD]], expr: !DIExpression())
; CHECK-DAG: [[GLOBAL_MD]] = !{[[B_MDE]], [[C_MDE]], [[A_MDE]], [[E_MDE]]}

@a = internal addrspace(2) constant [2 x i32] [i32 0, i32 1], align 4, !dbg !0
@c = internal addrspace(1) global i32 0, align 4, !dbg !10
@e = internal addrspace(1) global i32 0, align 4, !dbg !12
@b = common addrspace(1) global i32 0, align 4, !dbg !6

; Function Attrs: noinline nounwind
define spir_kernel void @test_(i32 addrspace(1)* %dst) #0 !dbg !22 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %aa = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !27, metadata !DIExpression()), !dbg !28
  call void @llvm.dbg.declare(metadata i32* %aa, metadata !29, metadata !DIExpression()), !dbg !30
  %0 = getelementptr inbounds [2 x i32], [2 x i32] addrspace(2)* @a, i64 0, i64 1
  %1 = load i32, i32 addrspace(2)* %0, align 4, !dbg !31
  store i32 %1, i32* %aa, align 4, !dbg !30
  store i32 3, i32 addrspace(1)* @c, align 4, !dbg !32
  ret void, !dbg !33
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!17, !18}
!igc.functions = !{!19}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "a", scope: !2, file: !8, line: 1, type: !14, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "<stdin>", directory: "dir")
!4 = !{}
!5 = !{!6, !10, !0, !12}
!6 = !DIGlobalVariableExpression(var: !7, expr: !DIExpression())
!7 = distinct !DIGlobalVariable(name: "b", scope: !2, file: !8, line: 2, type: !9, isLocal: true, isDefinition: true)
!8 = !DIFile(filename: "UndefinedReferencesPass.ll", directory: "dir")
!9 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!10 = !DIGlobalVariableExpression(var: !11, expr: !DIExpression())
!11 = distinct !DIGlobalVariable(name: "c", scope: !2, file: !8, line: 3, type: !9, isLocal: true, isDefinition: true)
!12 = !DIGlobalVariableExpression(var: !13, expr: !DIExpression())
!13 = distinct !DIGlobalVariable(name: "e", scope: !2, file: !8, line: 4, type: !9, isLocal: true, isDefinition: true)
!14 = !DICompositeType(tag: DW_TAG_array_type, baseType: !9, size: 64, elements: !15)
!15 = !{!16}
!16 = !DISubrange(count: 2)
!17 = !{i32 2, !"Dwarf Version", i32 4}
!18 = !{i32 2, !"Debug Info Version", i32 3}
!19 = !{void (i32 addrspace(1)*)* @test_, !20}
!20 = !{!21}
!21 = !{!"function_type", i32 0}
!22 = distinct !DISubprogram(name: "test_", scope: null, file: !8, line: 6, type: !23, flags: DIFlagPrototyped, unit: !2, templateParams: !4, retainedNodes: !4)
!23 = !DISubroutineType(types: !24)
!24 = !{!25, !26}
!25 = !DIBasicType(name: "int", size: 4)
!26 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !9, size: 64)
!27 = !DILocalVariable(name: "dst", arg: 1, scope: !22, file: !8, line: 6, type: !26)
!28 = !DILocation(line: 6, column: 33, scope: !22)
!29 = !DILocalVariable(name: "aa", scope: !22, file: !8, line: 8, type: !9)
!30 = !DILocation(line: 8, column: 9, scope: !22)
!31 = !DILocation(line: 8, column: 14, scope: !22)
!32 = !DILocation(line: 9, column: 7, scope: !22)
!33 = !DILocation(line: 10, column: 1, scope: !22)
