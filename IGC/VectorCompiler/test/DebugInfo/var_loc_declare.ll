;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test checks that @llvm.dbg.declare will transform to location in dwarf
;
; RUN: llc %s -march=genx64 -mcpu=Gen9 -mattr=+ocl_runtime \
; RUN: -vc-enable-dbginfo-dumps -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-debug -generateDebugInfo' -o /dev/null
;
; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_test_dwarf.elf | FileCheck %s

; CHECK:      DW_AT_name        : tmp_val
; CHECK-NEXT: DW_AT_decl_file   : 1
; CHECK-NEXT: DW_AT_decl_line   : 6
; CHECK-NEXT: DW_AT_type        : <{{0x[0-9a-f]+}}>
; CHECK-NEXT: DW_AT_location    : 0x[[#LOC_OFFSET:]] (location list)

; CHECK: Contents of the .debug_loc section:
; CHECK-DAG: [[#LOC_OFFSET]] {{[^(]+}}(DW_OP_reg[[#]] (r[[#]]); DW_OP_const1u: 64; DW_OP_const1u: 64; DW_OP_INTEL_push_bit_piece_stack)


target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: noinline nounwind
define spir_kernel void @test(i32* %0, i64 %privBase) local_unnamed_addr #0 !dbg !22 {
  call void @llvm.dbg.declare(metadata i32* %0, metadata !27, metadata !DIExpression()), !dbg !30
  ret void, !dbg !33
}

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #2

attributes #0 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!opencl.spir.version = !{!13}
!genx.kernels = !{!15}
!genx.kernel.internal = !{!20}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "the_file.cpp", directory: "/the_directory")
!4 = !{}
!5 = !{!6}
!6 = !DIGlobalVariableExpression(var: !7, expr: !DIExpression())
!7 = distinct !DIGlobalVariable(name: "init", scope: !2, file: !3, line: 3, type: !8, isLocal: true, isDefinition: true)
!8 = !DICompositeType(tag: DW_TAG_array_type, baseType: !9, size: 512, elements: !10)
!9 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!10 = !{!11}
!11 = !DISubrange(count: 16)
!12 = !{i32 0, i32 0}
!13 = !{i32 1, i32 2}
!15 = !{void (i32*, i64)* @test, !"test", !16, i32 0, !17, !18, !19, i32 0}
!16 = !{i32 0, i32 96}
!17 = !{i32 72, i32 64}
!18 = !{i32 0}
!19 = !{!"svmptr_t"}
!20 = !{void (i32*, i64)* @test, !12, !21, !4, !13}
!21 = !{i32 0, i32 1}
!22 = distinct !DISubprogram(name: "test", scope: null, file: !3, line: 6, type: !23, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !26)
!23 = !DISubroutineType(types: !24)
!24 = !{null, !25}
!25 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !9, size: 64)
!26 = !{!27}
!27 = !DILocalVariable(name: "tmp_val", arg: 1, scope: !22, file: !3, line: 6, type: !25)
!29 = !DICompositeType(tag: DW_TAG_array_type, baseType: !9, size: 512, flags: DIFlagVector, elements: !10)
!30 = !DILocation(line: 0, scope: !22)
!33 = !DILocation(line: 9, column: 1, scope: !22)
