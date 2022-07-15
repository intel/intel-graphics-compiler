;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-predefined-constant-resolve -S < %s | FileCheck %s
; ------------------------------------------------
; PredefinedConstantResolving
; ------------------------------------------------
; This test checks that PredefinedConstantResolving pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __constant int ga = 42;
;
; __kernel void test_const(__global uint* dst)
; {
;   int la = ga;
;   dst[0] = la;
; }
;
; ------------------------------------------------

; CHECK: @ga = {{.*}} !dbg [[GA_MD:![0-9]*]]
; CHECK: define spir_kernel void @test_const
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata [[DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DST_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.value
; CHECK-SAME: i32 {{[A-z0-9%]*}}
; CHECK-SAME: metadata [[LA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LA_LOC:![0-9]*]]
;
; CHECK: store {{.*}} !dbg [[STORE_LOC:![0-9]*]]

@ga = addrspace(2) constant i32 42, align 4, !dbg !0

define spir_kernel void @test_const(i32 addrspace(1)* %dst) #0 !dbg !11 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !19, metadata !DIExpression()), !dbg !20
  %0 = load i32, i32 addrspace(2)* @ga, align 4, !dbg !21
  call void @llvm.dbg.value(metadata i32 %0, metadata !22, metadata !DIExpression()), !dbg !23
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !24
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0, !dbg !24
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !25
  ret void, !dbg !26
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "PredefinedConstantResolving.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_const", scope: null, file: [[FILE]], line: 3
; CHECK-DAG: [[GA_MD]] = !DIGlobalVariableExpression(var: [[GA_VAR:![0-9]*]], expr: !DIExpression())
; CHECK-DAG: [[GA_VAR]] = distinct !DIGlobalVariable(name: "ga", scope: [[GA_SCOPE:![0-9]*]], file: !6, line: 1
; CHECK-DAG: [[GA_SCOPE]] = distinct !DICompileUnit(language: DW_LANG_C99
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 3, column: 41, scope: [[SCOPE]])
; CHECK-DAG: [[LA_MD]] = !DILocalVariable(name: "la", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[LA_LOC]] = !DILocation(line: 5, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 6, column: 10, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!8, !9, !10}
!igc.functions = !{}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "ga", scope: !2, file: !6, line: 1, type: !7, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "<stdin>", directory: "/")
!4 = !{}
!5 = !{!0}
!6 = !DIFile(filename: "PredefinedConstantResolving.ll", directory: "/")
!7 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!8 = !{i32 2, !"Dwarf Version", i32 4}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = !{i32 1, !"wchar_size", i32 4}
!11 = distinct !DISubprogram(name: "test_const", scope: null, file: !6, line: 3, type: !12, flags: DIFlagPrototyped, unit: !2, templateParams: !4, retainedNodes: !4)
!12 = !DISubroutineType(types: !13)
!13 = !{!14, !15}
!14 = !DIBasicType(name: "int", size: 4)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64)
!16 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !17, baseType: !18)
!17 = !DIFile(filename: "header.h", directory: "/")
!18 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!19 = !DILocalVariable(name: "dst", arg: 1, scope: !11, file: !6, line: 3, type: !15)
!20 = !DILocation(line: 3, column: 41, scope: !11)
!21 = !DILocation(line: 5, column: 12, scope: !11)
!22 = !DILocalVariable(name: "la", scope: !11, file: !6, line: 5, type: !7)
!23 = !DILocation(line: 5, column: 7, scope: !11)
!24 = !DILocation(line: 6, column: 3, scope: !11)
!25 = !DILocation(line: 6, column: 10, scope: !11)
!26 = !DILocation(line: 7, column: 1, scope: !11)
