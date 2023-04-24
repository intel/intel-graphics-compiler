;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Tests that rdregions inside bales may have debug-values.
;
; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_kernel_dwarf.elf | FileCheck %s

; CHECK:      (DW_TAG_variable)
; CHECK-NEXT: DW_AT_name        : a
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_location    : {{[^(]+}}
; CHECK-SAME: (DW_OP_reg[[REG_A:[0-9]+]]
; CHECK-SAME: (r[[REG_A]]); DW_OP_bit_piece: size: 32 offset: [[OFF_A:[0-9]+]] ;
; CHECK-SAME: DW_OP_reg[[REG_A]] (r[[REG_A]]); DW_OP_bit_piece: size: 32 offset: [[OFF_A]] ;
; CHECK-SAME: DW_OP_reg[[REG_A]] (r[[REG_A]]); DW_OP_bit_piece: size: 32 offset: [[OFF_A]] ;
; CHECK-SAME: DW_OP_reg[[REG_A]] (r[[REG_A]]); DW_OP_bit_piece: size: 32 offset: [[OFF_A]] )

; CHECK:      (DW_TAG_variable)
; CHECK-NEXT: DW_AT_name        : b
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_location    : {{[^(]+}}
; CHECK-SAME: (DW_OP_reg[[REG_B:[0-9]+]]
; CHECK-SAME: (r[[REG_B]]); DW_OP_bit_piece: size: 16 offset: [[OFF_B:[0-9]+]] ;
; CHECK-SAME: DW_OP_reg[[REG_B]] (r[[REG_B]]); DW_OP_bit_piece: size: 16 offset: [[OFF_B]] ;
; CHECK-SAME: DW_OP_reg[[REG_B]] (r[[REG_B]]); DW_OP_bit_piece: size: 16 offset: [[OFF_B]] ;
; CHECK-SAME: DW_OP_reg[[REG_B]] (r[[REG_B]]); DW_OP_bit_piece: size: 16 offset: [[OFF_B]] )

; ModuleID = 'Deserialized LLVM Module'
target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @kernel(i32 %arg, i32 %arg1, i16 signext %arg2, i64 %privBase) local_unnamed_addr #0 !dbg !20 {
  %bitcast = bitcast i32 %arg1 to <1 x i32>
  %rdregioni4 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> %bitcast, i32 0, i32 4, i32 0, i16 0, i32 undef)
  call void @llvm.dbg.value(metadata <4 x i32> %rdregioni4, metadata !31, metadata !DIExpression()), !dbg !38
  %bitcast3 = bitcast i16 %arg2 to <1 x i16>
  %rdregioni = call <4 x i16> @llvm.genx.rdregioni.v4i16.v1i16.i16(<1 x i16> %bitcast3, i32 0, i32 4, i32 0, i16 0, i32 undef)
  call void @llvm.dbg.value(metadata <4 x i16> %rdregioni, metadata !35, metadata !DIExpression()), !dbg !38
  %sext = sext <4 x i16> %rdregioni to <4 x i32>
  %mul = mul <4 x i32> %rdregioni4, %sext
  tail call void @llvm.genx.oword.st.v4i32(i32 %arg, i32 0, <4 x i32> %mul)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v4i32(i32, i32, <4 x i32>) #1

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !45 <4 x i16> @llvm.genx.rdregioni.v4i16.v1i16.i16(<1 x i16>, i32, i32, i32, i16, i32) #3

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !45 <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32>, i32, i32, i32, i16, i32) #3

attributes #0 = { noinline nounwind "CMGenxMain" }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone speculatable willreturn }
attributes #3 = { nounwind readnone }
attributes #4 = { nounwind writeonly }

!llvm.module.flags = !{!0, !1, !2}
!llvm.dbg.cu = !{!3}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!6}
!opencl.spir.version = !{!7, !8, !8}
!opencl.ocl.version = !{!6, !8, !8}
!opencl.used.extensions = !{!5}
!opencl.used.optional.core.features = !{!5}
!spirv.Generator = !{!9}
!genx.kernels = !{!10}
!genx.kernel.internal = !{!15}
!llvm.ident = !{!19, !19}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !4, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !5)
!4 = !DIFile(filename: "test.cm", directory: "./")
!5 = !{}
!6 = !{i32 0, i32 0}
!7 = !{i32 1, i32 2}
!8 = !{i32 2, i32 0}
!9 = !{i16 6, i16 14}
!10 = !{void (i32, i32, i16, i64)* @kernel, !"kernel", !11, i32 0, !12, !13, !14, i32 0}
!11 = !{i32 2, i32 0, i32 0, i32 96}
!12 = !{i32 72, i32 80, i32 84, i32 64}
!13 = !{i32 0, i32 0, i32 0}
!14 = !{!"buffer_t read_write", !"", !""}
!15 = !{void (i32, i32, i16, i64)* @kernel, !16, !17, !5, !18}
!16 = !{i32 0, i32 0, i32 0, i32 0}
!17 = !{i32 0, i32 1, i32 2, i32 3}
!18 = !{i32 1, i32 -1, i32 -1, i32 2}
!19 = !{!"Ubuntu"}
!20 = distinct !DISubprogram(name: "kernel", scope: null, file: !4, line: 4, type: !21, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !3, templateParams: !5, retainedNodes: !27)
!21 = !DISubroutineType(types: !22)
!22 = !{null, !23, !25, !26}
!23 = !DIDerivedType(tag: DW_TAG_typedef, name: "SurfaceIndex_buffer_t", file: !4, baseType: !24)
!24 = !DIBasicType(name: "SurfaceIndex", size: 32, encoding: DW_ATE_unsigned)
!25 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!26 = !DIBasicType(name: "short", size: 16, encoding: DW_ATE_signed)
!27 = !{!28, !31, !35}
!28 = !DILocalVariable(name: "status_buf", arg: 1, scope: !20, file: !4, line: 4, type: !23)
!31 = !DILocalVariable(name: "a", scope: !20, file: !4, line: 5, type: !32)
!32 = !DICompositeType(tag: DW_TAG_array_type, baseType: !25, size: 128, flags: DIFlagVector, elements: !33)
!33 = !{!34}
!34 = !DISubrange(count: 4)
!35 = !DILocalVariable(name: "b", scope: !20, file: !4, line: 6, type: !36)
!36 = !DICompositeType(tag: DW_TAG_array_type, baseType: !26, size: 64, flags: DIFlagVector, elements: !33)
!38 = !DILocation(line: 0, scope: !20)
!45 = !{i32 7743}
