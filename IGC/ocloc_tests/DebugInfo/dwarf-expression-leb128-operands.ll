;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; DIExpression operands that DWARF encodes as LEB128 must be emitted as LEB128.
; Each value below needs more than one LEB128 byte, so a fixed-size encoding
; corrupts the expression.

; UNSUPPORTED: sys32
; REQUIRES: dg2-supported, llvm-16-plus, regkeys, oneapi-readelf

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=info,loc %t_OCL_simd16_leb128_operands.elf | FileCheck %s --implicit-check-not="User defined location op"

; CHECK-DAG: DW_OP_INTEL_regval_bits: 32; DW_OP_plus_uconst: 192; DW_OP_stack_value
; CHECK-DAG: DW_OP_INTEL_regval_bits: 32; DW_OP_constu: 300; DW_OP_plus; DW_OP_stack_value
; CHECK-DAG: DW_OP_INTEL_regval_bits: 32; DW_OP_consts: -200; DW_OP_plus; DW_OP_stack_value

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @leb128_operands(ptr addrspace(1) %out, i32 %a) !dbg !6 !intel_reqd_sub_group_size !20 {
entry:
  %v = add i32 %a, 1, !dbg !14
  call void @llvm.dbg.value(metadata i32 %v, metadata !10, metadata !DIExpression(DW_OP_plus_uconst, 192, DW_OP_stack_value)), !dbg !14
  call void @llvm.dbg.value(metadata i32 %v, metadata !12, metadata !DIExpression(DW_OP_constu, 300, DW_OP_plus, DW_OP_stack_value)), !dbg !14
  call void @llvm.dbg.value(metadata i32 %v, metadata !13, metadata !DIExpression(DW_OP_consts, 18446744073709551416, DW_OP_plus, DW_OP_stack_value)), !dbg !14
  %w = mul i32 %v, %a, !dbg !15
  store i32 %w, ptr addrspace(1) %out, align 4, !dbg !15
  %x = add i32 %w, %v, !dbg !16
  %p = getelementptr inbounds i32, ptr addrspace(1) %out, i64 1, !dbg !16
  store i32 %x, ptr addrspace(1) %p, align 4, !dbg !16
  ret void, !dbg !17
}

declare void @llvm.dbg.value(metadata, metadata, metadata)

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "test", isOptimized: true, emissionKind: FullDebug, enums: !3)
!2 = !DIFile(filename: "test.cpp", directory: "/tmp")
!3 = !{}
!6 = distinct !DISubprogram(name: "leb128_operands", scope: null, file: !2, line: 1, type: !7, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !1, retainedNodes: !3)
!7 = !DISubroutineType(types: !8)
!8 = !{null}
!9 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!10 = !DILocalVariable(name: "plus", scope: !6, file: !2, line: 2, type: !9)
!12 = !DILocalVariable(name: "constu", scope: !6, file: !2, line: 3, type: !9)
!13 = !DILocalVariable(name: "consts", scope: !6, file: !2, line: 4, type: !9)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 5, column: 1, scope: !6)
!16 = !DILocation(line: 6, column: 1, scope: !6)
!17 = !DILocation(line: 7, column: 1, scope: !6)
!20 = !{i32 16}
