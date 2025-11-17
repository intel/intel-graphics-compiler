;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Tests that non stack variables on O2 aren't inlined.
; With optimizations disabled there's no guarantee that the offsets in registers will live throughout the entire variable lifetime.

; REQUIRES: regkeys, oneapi-readelf, llvm-16-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t
; RUN: ocloc compile -llvm_input -file %t -device dg2 -options "-g -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'"
; RUN: oneapi-readelf --debug-dump %t_OCL_simd32_foo.elf | FileCheck %s

; CHECK: DW_AT_name : test_filter_tile
; CHECK-NEXT: DW_AT_decl_file : 1
; CHECK-NEXT: DW_AT_decl_line : 454
; CHECK-NEXT: DW_AT_type : {{.*}}
; CHECK-NEXT: DW_AT_location : 0 (location list)

; CHECK: Contents of the .debug_loc section:
; CHECK: {{0+}} {{[0-9A-Fa-f]+}} {{[0-9A-Fa-f]+}} (DW_OP_INTEL_push_simd_lane; DW_OP_lit16; DW_OP_ge; DW_OP_bra: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit2; DW_OP_shr; DW_OP_plus_uconst: {{[0-9]+}}; DW_OP_INTEL_push_simd_lane; DW_OP_lit3; DW_OP_and; DW_OP_const1u: 64; DW_OP_mul; DW_OP_INTEL_regval_bits: 64; DW_OP_skip: 15; DW_OP_INTEL_push_simd_lane; DW_OP_lit16; DW_OP_minus; DW_OP_lit2; DW_OP_shr; DW_OP_plus_uconst: {{[0-9]+}}; DW_OP_INTEL_push_simd_lane; DW_OP_lit3; DW_OP_and; DW_OP_const1u: 64; DW_OP_mul; DW_OP_INTEL_regval_bits: 64)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @foo(i64 %0) {
  %2 = alloca [81 x float], align 4
  call void @llvm.dbg.declare(metadata ptr %2, metadata !11, metadata !DIExpression(DW_OP_constu, 4, DW_OP_swap, DW_OP_xderef)), !dbg !19
  %3 = call spir_func i64 @_Z13get_global_idj(i32 0), !dbg !20
  %4 = trunc i64 %3 to i32
  %5 = icmp sgt i32 %4, 0
  br i1 %5, label %7, label %6

6:                                                ; preds = %1
  store i8 0, ptr %2, align 4
  %.phi.trans.insert = getelementptr [81 x float], ptr %2, i64 0, i64 %0
  %.pre = load float, ptr %.phi.trans.insert, align 4
  br label %.loopexit2

.loopexit2:                                       ; preds = %.loopexit2, %6
  store float %.pre, ptr addrspace(1) null, align 4294967296
  br label %.loopexit2

7:                                                ; preds = %1
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0}
!llvm.dbg.cu = !{!1}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !2, producer: "clang based Intel(R) oneAPI DPC++/C++ Compiler 2026.0.0 (2026.x.0.20250920)", isOptimized: false, flags: " --driver-mode=g++ --intel -c -O2 -g -fiopenmp --offload-targets=spir64 -fno-exceptions -D GPU_OFFLOAD_NOEH -D GPU_OFFLOAD_WA_CLANG -D GPU_OFFLOAD_WA_DT convolution.cpp -fveclib=SVML -shared -fPIC", runtimeVersion: 0, emissionKind: FullDebug, globals: !3, imports: !10)
!2 = !DIFile(filename: "convolution.cpp", directory: "/netbatch/alTC98419_00/exp/ompo_kernels_small_gpuCpp~2-2/opt_speed_debug_igen")
!3 = !{!4, !8}
!4 = !DIGlobalVariableExpression(var: !5, expr: !DIExpression())
!5 = distinct !DIGlobalVariable(name: "src_tile_width", scope: !1, file: !2, line: 464, type: !6, isLocal: true, isDefinition: true)
!6 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !7)
!7 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!8 = !DIGlobalVariableExpression(var: !9, expr: !DIExpression())
!9 = distinct !DIGlobalVariable(name: "src_tile_h_offset", scope: !1, file: !2, line: 463, type: !6, isLocal: true, isDefinition: true)
!10 = !{}
!11 = !DILocalVariable(name: "test_filter_tile", scope: !12, file: !2, line: 454, type: !17)
!12 = distinct !DILexicalBlock(scope: !13, file: !2, line: 452, column: 196)
!13 = distinct !DILexicalBlock(scope: !14, file: !2, line: 447, column: 196)
!14 = distinct !DISubprogram(name: "_ZN19ConvolutionLocalsAN15execute_offloadEPfi.extracted", scope: null, file: !2, line: 447, type: !15, scopeLine: 447, flags: DIFlagArtificial, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized | DISPFlagMainSubprogram, unit: !1, templateParams: !10, retainedNodes: !10)
!15 = !DISubroutineType(types: !16)
!16 = !{null}
!17 = !DICompositeType(tag: DW_TAG_array_type, baseType: !18, size: 2592, elements: !10)
!18 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!19 = !DILocation(line: 454, column: 15, scope: !12)
!20 = !DILocation(line: 452, column: 1, scope: !12)

