;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 -mattr=+ocl_runtime \
; RUN: -vc-enable-dbginfo-dumps -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_test_dwarf.elf | FileCheck %s

; CHECK:      DW_AT_name        : buff_address
; CHECK-NEXT: DW_AT_decl_file   : 1
; CHECK-NEXT: DW_AT_decl_line   : 6
; CHECK-NEXT: DW_AT_type        : <{{0x[0-9a-f]+}}>
; CHECK-NEXT: DW_AT_location    : 0x[[BUF_LOC:[0-9a-f]+]] (location list)

; CHECK:      DW_AT_name        : artificial_vect
; CHECK-NEXT: DW_AT_decl_file   : 1
; CHECK-NEXT: DW_AT_decl_line   : 777
; CHECK-NEXT: DW_AT_type        : <[[VECT_TYPE:0x[0-9a-f]+]]>
; CHECK-NEXT: DW_AT_location    : 0x[[VECT_LOC:[0-9a-f]+]] (location list)

; CHECK:      DW_AT_name        : data
; CHECK-NEXT: DW_AT_decl_file   : 1
; CHECK-NEXT: DW_AT_decl_line   : 7
; CHECK-NEXT: DW_AT_type        : <[[VECT_TYPE]]>
; CHECK-NEXT: DW_AT_const_value : 64 byte block: 0 0 0 0 1 0 0 0 2 0 0 0 3 0 0 0 4 0 0 0 5 0 0 0 6 0 0 0 7 0 0 0 8 0 0 0 9 0 0 0 a 0 0 0 b 0 0 0 c 0 0 0 d 0 0 0 e 0 0 0 f 0 0 0

; debug_loc section
; CHECK-DAG: [[BUF_LOC]] {{[^(]+}}(DW_OP_reg[[#REG_PTR:]] (r[[#]]); DW_OP_bit_piece: size: 64 offset: [[#OFFSET:]] )
; CHECK-DAG: [[VECT_LOC]] {{[^(]+}}(DW_OP_reg[[#REG_PTR]] (r[[#REG_PTR]]); DW_OP_const1u: [[#OFFSET]]; DW_OP_const1u: 64;  DW_OP_INTEL_push_bit_piece_stack)

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test(i32* %0, i64 %privBase) local_unnamed_addr #0 !dbg !22 {
  call void @llvm.dbg.value(metadata i32* %0, metadata !34, metadata !DIExpression(DW_OP_deref)), !dbg !32
  call void @llvm.dbg.value(metadata i32* %0, metadata !27, metadata !DIExpression()), !dbg !30
  call void @llvm.dbg.value(metadata <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, metadata !28, metadata !DIExpression()), !dbg !30
  %2 = ptrtoint i32* %0 to i64, !dbg !31
  tail call void @llvm.genx.svm.block.st.i64.v16i32(i64 %2, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>), !dbg !32
  ret void, !dbg !33
}

; Function Attrs: nounwind
declare void @llvm.genx.svm.block.st.i64.v16i32(i64, <16 x i32>) #1

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

attributes #0 = { noinline nounwind "CMGenxMain" "oclrt"="1" }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!12}
!opencl.spir.version = !{!13}
!opencl.ocl.version = !{!12}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!14}
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
!14 = !{i16 6, i16 14}
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
!26 = !{!27, !28, !34}
!27 = !DILocalVariable(name: "buff_address", arg: 1, scope: !22, file: !3, line: 6, type: !25)
!28 = !DILocalVariable(name: "data", scope: !22, file: !3, line: 7, type: !29)
!34 = !DILocalVariable(name: "artificial_vect", scope: !22, file: !3, line: 777, type: !29)
!29 = !DICompositeType(tag: DW_TAG_array_type, baseType: !9, size: 512, flags: DIFlagVector, elements: !10)
!30 = !DILocation(line: 0, scope: !22)
!31 = !DILocation(line: 8, column: 22, scope: !22)
!32 = !DILocation(line: 8, column: 3, scope: !22)
!33 = !DILocation(line: 9, column: 1, scope: !22)
