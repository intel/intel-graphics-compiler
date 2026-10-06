;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; With stack calls, private variables live on the stack and their locations are FP-relative.
; With optimizations enabled the debug emitter still needs the frame pointer to build them.

; UNSUPPORTED: sys32
; REQUIRES: dg2-supported, llvm-16-plus, regkeys, oneapi-readelf

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd8_stack_variable.elf | FileCheck %s

; CHECK:      DW_AT_name{{.*}}: arr
; CHECK:      DW_AT_location{{.*}}(location list)
; CHECK:      Contents of the .debug_loc section:
; CHECK:      DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit16; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: 0)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @stack_variable(ptr addrspace(1) %out) !dbg !10 {
  %arr = alloca [4 x i32], align 4
  call void @llvm.dbg.declare(metadata ptr %arr, metadata !14, metadata !DIExpression()), !dbg !17
  call spir_func void @fill(ptr %arr), !dbg !17
  %p = getelementptr [4 x i32], ptr %arr, i64 0, i64 1
  %v = load i32, ptr %p, align 4, !dbg !17
  store i32 %v, ptr addrspace(1) %out, align 4, !dbg !17
  ret void, !dbg !17
}

define spir_func void @fill(ptr %p) #0 !dbg !20 {
  store i32 1, ptr %p, align 4, !dbg !21
  %q = getelementptr i32, ptr %p, i64 1
  store i32 2, ptr %q, align 4, !dbg !21
  ret void, !dbg !21
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)

attributes #0 = { noinline "visaStackCall" }

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "test", isOptimized: true, emissionKind: FullDebug, enums: !3)
!2 = !DIFile(filename: "test.cpp", directory: "/tmp")
!3 = !{}
!4 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!5 = distinct !DISubroutineType(types: !3)
!6 = !DICompositeType(tag: DW_TAG_array_type, baseType: !4, size: 128, elements: !7)
!7 = !{!8}
!8 = !DISubrange(count: 4)
!10 = distinct !DISubprogram(name: "stack_variable", scope: null, file: !2, line: 10, type: !5, scopeLine: 10, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !1, retainedNodes: !3)
!14 = !DILocalVariable(name: "arr", scope: !10, file: !2, line: 11, type: !6)
!17 = !DILocation(line: 12, column: 5, scope: !10)
!20 = distinct !DISubprogram(name: "fill", scope: null, file: !2, line: 20, type: !5, scopeLine: 20, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !1, retainedNodes: !3)
!21 = !DILocation(line: 21, column: 5, scope: !20)
