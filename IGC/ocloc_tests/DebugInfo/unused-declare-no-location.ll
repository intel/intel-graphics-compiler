;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; A declare's operand is the variable's address, so an unused one is bad and should be dropped.

; UNSUPPORTED: sys32
; REQUIRES: dg2-supported, llvm-16-plus, regkeys, oneapi-readelf

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump=info %t_OCL_simd32_unused_declares.elf | FileCheck %s

; CHECK:      DW_TAG_variable
; CHECK-NEXT: DW_AT_name{{.*}}: varNull
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; CHECK-NEXT: DW_AT_type
; CHECK-NOT:  DW_AT_const_value

; CHECK-NEXT: DW_TAG_variable
; CHECK-NEXT: DW_AT_name{{.*}}: varDead
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; CHECK-NEXT: DW_AT_type
; CHECK-NOT:  DW_AT_const_value

; CHECK-NEXT: DW_TAG_variable
; CHECK-NEXT: DW_AT_name{{.*}}: varLive
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_const_value

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @unused_declares(ptr addrspace(1) %out) !dbg !10 {
  %dead = alloca i64, align 8
  %live = alloca i64, align 8
  call void @llvm.dbg.declare(metadata ptr null, metadata !14, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata ptr %dead, metadata !15, metadata !DIExpression()), !dbg !17
  call void @llvm.dbg.declare(metadata ptr %live, metadata !16, metadata !DIExpression()), !dbg !17
  store i64 42, ptr %live, align 8, !dbg !17
  %v = load i64, ptr %live, align 8, !dbg !17
  store i64 %v, ptr addrspace(1) %out, align 8, !dbg !17
  ret void, !dbg !17
}

declare void @llvm.dbg.declare(metadata, metadata, metadata)

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "test", isOptimized: false, emissionKind: FullDebug, enums: !3)
!2 = !DIFile(filename: "test.cpp", directory: "/tmp")
!3 = !{}
!4 = !DIBasicType(name: "long", size: 64, encoding: DW_ATE_signed)
!5 = distinct !DISubroutineType(types: !3)
!10 = distinct !DISubprogram(name: "unused_declares", scope: null, file: !2, line: 10, type: !5, scopeLine: 10, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !1, retainedNodes: !3)
!14 = !DILocalVariable(name: "varNull", scope: !10, file: !2, line: 11, type: !4)
!15 = !DILocalVariable(name: "varDead", scope: !10, file: !2, line: 12, type: !4)
!16 = !DILocalVariable(name: "varLive", scope: !10, file: !2, line: 13, type: !4)
!17 = !DILocation(line: 14, column: 5, scope: !10)
