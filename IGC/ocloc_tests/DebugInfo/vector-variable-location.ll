;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Each vector element is its own register value and piece; at SIMD16 an element spans two GRFs.
; UNSUPPORTED: sys32
; REQUIRES: regkeys, oneapi-readelf, dg2-supported, llvm-17-plus

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=loc %t_OCL_simd16_test.elf | FileCheck %s

; CHECK: DW_OP_plus_uconst: [[#REG:]];{{.*}} DW_OP_INTEL_regval_bits: 32; DW_OP_stack_value; DW_OP_piece: 4;
; CHECK-SAME: DW_OP_plus_uconst: [[#REG+2]];{{.*}} DW_OP_INTEL_regval_bits: 32; DW_OP_stack_value; DW_OP_piece: 4)
; The element pieces cover the whole fragment, so no bit piece follows them.
; CHECK: DW_OP_plus_uconst: [[#REG]];{{.*}} DW_OP_INTEL_regval_bits: 32; DW_OP_stack_value; DW_OP_piece: 4; {{.*}} DW_OP_plus_uconst: [[#REG+2]];{{.*}} DW_OP_INTEL_regval_bits: 32; DW_OP_stack_value; DW_OP_piece: 4)

target triple = "spir64-unknown-unknown"

define spir_kernel void @test(ptr addrspace(1) %p) !dbg !3 !intel_reqd_sub_group_size !8 {
  %gid = call spir_func i64 @_Z13get_global_idj(i32 0), !dbg !7
  %addr = getelementptr inbounds <2 x float>, ptr addrspace(1) %p, i64 %gid, !dbg !7
  %vector = load <2 x float>, ptr addrspace(1) %addr, align 8, !dbg !7
  call void @llvm.dbg.value(metadata <2 x float> %vector, metadata !6, metadata !DIExpression()), !dbg !7
  call void @llvm.dbg.value(metadata <2 x float> %vector, metadata !9, metadata !DIExpression(DW_OP_LLVM_fragment, 0, 64)), !dbg !7
  %twice = fadd <2 x float> %vector, %vector, !dbg !7
  store <2 x float> %twice, ptr addrspace(1) %addr, align 8, !dbg !7
  ret void, !dbg !7
}

declare spir_func i64 @_Z13get_global_idj(i32)
declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_OpenCL, file: !2, emissionKind: FullDebug)
!2 = !DIFile(filename: "test.cl", directory: "/")
!3 = distinct !DISubprogram(name: "test", file: !2, type: !4, spFlags: DISPFlagDefinition, unit: !1)
!4 = !DISubroutineType(types: !{})
!5 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!6 = !DILocalVariable(name: "vector", scope: !3, type: !10)
!7 = !DILocation(line: 1, scope: !3)
!8 = !{i32 16}
!9 = !DILocalVariable(name: "fragment", scope: !3, type: !11)
!10 = !DICompositeType(tag: DW_TAG_array_type, baseType: !5, size: 64, elements: !{!12})
!11 = !DICompositeType(tag: DW_TAG_array_type, baseType: !5, size: 96, elements: !{!13})
!12 = !DISubrange(count: 2)
!13 = !DISubrange(count: 3)
