;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-move-private-memory-to-slm -S < %s | FileCheck %s
; ------------------------------------------------
; PrivateMemoryToSLM
; ------------------------------------------------
; This test checks that PrivateMemoryToSLM pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: @b = {{.*}}, !dbg [[GVAR_MD:![0-9]*]]
;
; CHECK: define spir_kernel void @test_slm
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; Line info for allocs makes little sense - not checked.
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata i32 {{.*}}%
; CHECK-SAME: metadata [[DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DST_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}%
; CHECK-SAME: metadata [[X_MD:![0-9]*]], metadata !DIExpression()), !dbg [[X_LOC:[!0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}%
; CHECK-SAME: metadata [[Y_MD:![0-9]*]], metadata !DIExpression()), !dbg [[Y_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: metadata {{.*}}%
; CHECK-SAME: metadata [[Z_MD:![0-9]*]], metadata !DIExpression()), !dbg [[Z_LOC:![0-9]*]]
;
; CHECK: store i32 {{.*}}, !dbg [[SX_LOC:![0-9]*]]
; CHECK: store i32 {{.*}}, !dbg [[SY_LOC:![0-9]*]]
; CHECK: store i32 {{.*}}, !dbg [[SZ_LOC:![0-9]*]]

@b = common addrspace(1) global i32 0, align 4, !dbg !0

define spir_kernel void @test_slm(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 !dbg !24 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8, !dbg !32
  %x = alloca i32, align 4, !dbg !33
  %y = alloca i32, align 4, !dbg !33
  %z = alloca i32, align 4, !dbg !33
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !34, metadata !DIExpression()), !dbg !35
  call void @llvm.dbg.declare(metadata i32* %x, metadata !36, metadata !DIExpression()), !dbg !37
  call void @llvm.dbg.declare(metadata i32* %y, metadata !38, metadata !DIExpression()), !dbg !39
  call void @llvm.dbg.declare(metadata i32* %z, metadata !40, metadata !DIExpression()), !dbg !41
  store i32 1, i32* %x, align 4, !dbg !42
  store i32 2, i32* %y, align 4, !dbg !43
  store i32 3, i32* %z, align 4, !dbg !44
  ret void, !dbg !32
}

; CHECK-DAG: [[GVAR_MD]] = !DIGlobalVariableExpression(var: [[GVAR:![0-9]]], expr: !DIExpression())
; CHECK-DAG: [[GVAR]] = distinct !DIGlobalVariable(name: "b"
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "PrivateMemoryToSLM.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_slm", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 38, scope: [[SCOPE]])
; CHECK-DAG: [[X_MD]] = !DILocalVariable(name: "x", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[X_LOC]] = !DILocation(line: 3, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[Y_MD]] = !DILocalVariable(name: "y", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[Y_LOC]] = !DILocation(line: 4, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[Z_MD]] = !DILocalVariable(name: "z", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[Z_LOC]] = !DILocation(line: 5, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[SX_LOC]] = !DILocation(line: 3, column: 12, scope: [[SCOPE]])
; CHECK-DAG: [[SY_LOC]] = !DILocation(line: 4, column: 11, scope: [[SCOPE]])
; CHECK-DAG: [[SZ_LOC]] = !DILocation(line: 5, column: 11, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!8, !9, !10}
!igc.functions = !{!11}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "b", scope: !2, file: !6, line: 2, type: !7, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "<stdin>", directory: "/")
!4 = !{}
!5 = !{!0}
!6 = !DIFile(filename: "PrivateMemoryToSLM.ll", directory: "/")
!7 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!8 = !{i32 2, !"Dwarf Version", i32 4}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = !{i32 1, !"wchar_size", i32 4}
!11 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i32)* @test_slm, !12}
!12 = !{!13, !14, !23}
!13 = !{!"function_type", i32 0}
!14 = !{!"implicit_arg_desc", !15, !16, !17, !18, !19, !20, !21}
!15 = !{i32 0}
!16 = !{i32 1}
!17 = !{i32 8}
!18 = !{i32 9}
!19 = !{i32 10}
!20 = !{i32 13}
!21 = !{i32 15, !22}
!22 = !{!"explicit_arg_num", i32 0}
!23 = !{!"thread_group_size", i32 1, i32 2, i32 1}
!24 = distinct !DISubprogram(name: "test_slm", scope: null, file: !6, line: 1, type: !25, flags: DIFlagPrototyped, unit: !2, templateParams: !4, retainedNodes: !4)
!25 = !DISubroutineType(types: !26)
!26 = !{!27, !28}
!27 = !DIBasicType(name: "int", size: 4)
!28 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !29, size: 64)
!29 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !30, baseType: !31)
!30 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!31 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!32 = !DILocation(line: 7, column: 1, scope: !24)
!33 = !DILocation(line: 77, column: 11, scope: !24)
!34 = !DILocalVariable(name: "dst", arg: 1, scope: !24, file: !6, line: 1, type: !28)
!35 = !DILocation(line: 1, column: 38, scope: !24)
!36 = !DILocalVariable(name: "x", scope: !24, file: !6, line: 3, type: !7)
!37 = !DILocation(line: 3, column: 7, scope: !24)
!38 = !DILocalVariable(name: "y", scope: !24, file: !6, line: 4, type: !7)
!39 = !DILocation(line: 4, column: 7, scope: !24)
!40 = !DILocalVariable(name: "z", scope: !24, file: !6, line: 5, type: !7)
!41 = !DILocation(line: 5, column: 7, scope: !24)
!42 = !DILocation(line: 3, column: 12, scope: !24)
!43 = !DILocation(line: 4, column: 11, scope: !24)
!44 = !DILocation(line: 5, column: 11, scope: !24)
