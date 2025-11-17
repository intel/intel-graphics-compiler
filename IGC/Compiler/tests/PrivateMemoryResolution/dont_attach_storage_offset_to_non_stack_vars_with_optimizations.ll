;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Tests that non stack variables don't have StorageOffset matadata attached that would cause them to get inlined.
; With optimizations disabled there's no guarantee that the offsets in registers will live throughout the entire variable lifetime.

; REQUIRES: regkeys, llvm-16-plus

; LLVM with opaque pointers:
; RUN: igc_opt --opaque-pointers --igc-private-mem-resolution --platformdg2 --regkey EnableOpaquePointersBackend=1 -S %s | FileCheck %s

; CHECK: call void @llvm.dbg.declare(metadata ptr %{{[.a-zA-Z0-9]+}}, metadata {{!?[0-9]*}}, metadata !DIExpression(DW_OP_constu, 4, DW_OP_swap, DW_OP_xderef)), !dbg !{{[0-9]+}}
; CHECK-NOT: !StorageOffset

source_filename = "dont_inline_non_stack_vars_with_optimizations.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent
define spir_kernel void @foo(i64 %0, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) #0 {
  %globalOffset.scalar = extractelement <3 x i32> %globalOffset, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar4 = extractelement <8 x i32> %r0, i64 1
  %2 = alloca [81 x float], align 4, !user_as_priv !483
  call void @llvm.dbg.declare(metadata ptr %2, metadata !484, metadata !DIExpression(DW_OP_constu, 4, DW_OP_swap, DW_OP_xderef)), !dbg !492
  %3 = mul i32 %enqueuedLocalSize.scalar, %r0.scalar4, !dbg !493
  %4 = zext i16 %localIdX to i32, !dbg !493
  %5 = add i32 %3, %4, !dbg !493
  %6 = add i32 %5, %globalOffset.scalar, !dbg !493
  %7 = icmp sgt i32 %6, 0
  br i1 %7, label %9, label %8

8:                                                ; preds = %1
  store i8 0, ptr %2, align 4, !user_as_priv !494
  %.phi.trans.insert = getelementptr [81 x float], ptr %2, i64 0, i64 %0, !user_as_priv !483
  %.pre = load float, ptr %.phi.trans.insert, align 4, !user_as_priv !494
  store float %.pre, ptr addrspace(1) null, align 4294967296
  br label %.loopexit2

.loopexit2:                                       ; preds = %.loopexit2, %8
  br label %.loopexit2

9:                                                ; preds = %1
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func i32 @__builtin_IB_get_group_id(i32 noundef) local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 noundef) local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func i32 @__builtin_IB_get_local_id_x() local_unnamed_addr #2

; Function Attrs: convergent mustprogress nofree nounwind willreturn memory(none)
declare spir_func i32 @__builtin_IB_get_global_offset(i32 noundef) local_unnamed_addr #2

declare i32 @printf(ptr addrspace(2), ...)

attributes #0 = { convergent }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { convergent mustprogress nofree nounwind willreturn memory(none) "no-trapping-math"="true" "stack-protector-buffer-size"="8" }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!igc.functions = !{!12}
!IGCMetadata = !{!23}

!0 = !{i32 2, !"Debug Info Version", i32 3}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !3, producer: "clang based Intel(R) oneAPI DPC++/C++ Compiler 2026.0.0 (2026.x.0.20250920)", isOptimized: false, flags: " --driver-mode=g++ --intel -c -O2 -g -fiopenmp --offload-targets=spir64 -fno-exceptions -D GPU_OFFLOAD_NOEH -D GPU_OFFLOAD_WA_CLANG -D GPU_OFFLOAD_WA_DT convolution.cpp -fveclib=SVML -shared -fPIC", runtimeVersion: 0, emissionKind: FullDebug, globals: !4, imports: !11)
!3 = !DIFile(filename: "convolution.cpp", directory: "/netbatch/alTC98419_00/exp/ompo_kernels_small_gpuCpp~2-2/opt_speed_debug_igen")
!4 = !{!5, !9}
!5 = !DIGlobalVariableExpression(var: !6, expr: !DIExpression())
!6 = distinct !DIGlobalVariable(name: "src_tile_width", scope: !2, file: !3, line: 464, type: !7, isLocal: true, isDefinition: true)
!7 = !DIDerivedType(tag: DW_TAG_const_type, baseType: !8)
!8 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!9 = !DIGlobalVariableExpression(var: !10, expr: !DIExpression())
!10 = distinct !DIGlobalVariable(name: "src_tile_h_offset", scope: !2, file: !3, line: 463, type: !7, isLocal: true, isDefinition: true)
!11 = !{}
!12 = !{ptr @foo, !13}
!13 = !{!14, !15}
!14 = !{!"function_type", i32 0}
!15 = !{!"implicit_arg_desc", !16, !17, !18, !19, !20, !21, !22}
!16 = !{i32 0}
!17 = !{i32 2}
!18 = !{i32 7}
!19 = !{i32 8}
!20 = !{i32 9}
!21 = !{i32 10}
!22 = !{i32 13}
!23 = !{!"ModuleMD", !25}
!25 = !{!"compOpt", !29}
!29 = !{!"OptDisable", i1 false}
!483 = !{!""}
!484 = !DILocalVariable(name: "test_filter_tile", scope: !485, file: !3, line: 454, type: !490)
!485 = distinct !DILexicalBlock(scope: !486, file: !3, line: 452, column: 196)
!486 = distinct !DILexicalBlock(scope: !487, file: !3, line: 447, column: 196)
!487 = distinct !DISubprogram(name: "_ZN19ConvolutionLocalsAN15execute_offloadEPfi.extracted", scope: null, file: !3, line: 447, type: !488, scopeLine: 447, flags: DIFlagArtificial, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized | DISPFlagMainSubprogram, unit: !2, templateParams: !11, retainedNodes: !11)
!488 = !DISubroutineType(types: !489)
!489 = !{null}
!490 = !DICompositeType(tag: DW_TAG_array_type, baseType: !491, size: 2592, elements: !11)
!491 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!492 = !DILocation(line: 454, column: 15, scope: !485)
!493 = !DILocation(line: 452, column: 1, scope: !485)
!494 = !{!"CannotUseSOALayout"}

