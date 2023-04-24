;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check rdregion-bale propagation (cm select calls) with complicated data
; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-debug -generateDebugInfo' -o /dev/null

; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_kernel_dwarf.elf | FileCheck %s

; CHECK:  DW_AT_name        : D5
; CHECK-NEXT:   DW_AT_decl_file
; CHECK-NEXT:   DW_AT_decl_line   : 18
; CHECK-NEXT:   DW_AT_type
; CHECK-NEXT:   DW_AT_location    :
; CHECK-SAME:     {{[^(]+}}(DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} )

; CHECK:  DW_AT_name        : D9
; CHECK-NEXT:  DW_AT_decl_file
; CHECK-NEXT:  DW_AT_decl_line   : 23
; CHECK-NEXT:  DW_AT_type
; CHECK-NEXT:  DW_AT_location    :
; CHECK-SAME:     {{[^(]+}}(DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} )

; CHECK:  DW_AT_name        : D13
; CHECK-NEXT:  DW_AT_decl_file
; CHECK-NEXT:  DW_AT_decl_line   : 29
; CHECK-NEXT:  DW_AT_type
; CHECK-NEXT:  DW_AT_location    :
; CHECK-SAME:     {{[^(]+}}(DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} )

; CHECK:  DW_AT_name        : D15
; CHECK-NEXT:  DW_AT_decl_file
; CHECK-NEXT:  DW_AT_decl_line   : 31
; CHECK-NEXT:  DW_AT_type
; CHECK-NEXT:  DW_AT_location    :
; CHECK-SAME:     {{[^(]+}}(DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} ;
; CHECK-SAME:     DW_OP_reg{{[^ ]+}} (r{{[^)]+}}); DW_OP_bit_piece: size: 32 offset: {{[0-9]+}} )


; ModuleID = 'Deserialized LLVM Module'
target triple = "genx64-unknown-unknown"

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @kernel(i32 %obuf, <128 x i32> %A, i64 %privBase) #0 !dbg !16 {
  %rdr.rows = call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> %A, i32 16, i32 16, i32 1, i16 128, i32 16)
  %rdr.cols = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> %rdr.rows, i32 16, i32 4, i32 1, i16 8, i32 16)
  call void @llvm.dbg.value(metadata <4 x i32> %rdr.cols, metadata !29, metadata !DIExpression()), !dbg !37
  %rdr.rows1 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> %A, i32 16, i32 16, i32 1, i16 192, i32 16)
  %rdr.cols2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> %rdr.rows1, i32 16, i32 4, i32 1, i16 12, i32 16)
  call void @llvm.dbg.value(metadata <4 x i32> %rdr.cols2, metadata !33, metadata !DIExpression()), !dbg !37
  %rdr.rows3 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> %A, i32 16, i32 16, i32 1, i16 0, i32 16)
  %rdr.cols4 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> %rdr.rows3, i32 16, i32 4, i32 2, i16 0, i32 16)
  call void @llvm.dbg.value(metadata <4 x i32> %rdr.cols4, metadata !34, metadata !DIExpression()), !dbg !37
  %rdr.cols6 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> %rdr.rows3, i32 16, i32 4, i32 4, i16 0, i32 16)
  call void @llvm.dbg.value(metadata <4 x i32> %rdr.cols6, metadata !35, metadata !DIExpression()), !dbg !37
  %add = add <4 x i32> %rdr.cols, %rdr.cols2
  %add7 = add <4 x i32> %add, %rdr.cols4
  %add8 = add <4 x i32> %add7, %rdr.cols6
  call void @llvm.genx.oword.st.v4i32(i32 1, i32 0, <4 x i32> %add8)
  ret void
}

; Function Attrs: nounwind readnone speculatable willreturn
; declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone
declare <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32>, i32, i32, i32, i16, i32) #2

; Function Attrs: nounwind readnone
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32>, i32, i32, i32, i16, i32) #2

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v4i32(i32, i32, <4 x i32>) #3

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!genx.kernels = !{!8}
!genx.kernel.internal = !{!12}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "src.cm", directory: "/tmp/tmp_cm_to_spv-81aad0")
!4 = !{}
!5 = !{i32 0, i32 0}
; !6 = !{i32 1, i32 2}
; !7 = !{i16 6, i16 14}
!8 = !{void (i32, <128 x i32>, i64)* @kernel, !"kernel", !9, i32 0, !10, !5, !11, i32 0}
!9 = !{i32 2, i32 0, i32 96}
!10 = !{i32 72, i32 96, i32 64}
!11 = !{!"buffer_t read_write", !""}
!12 = !{void (i32, <128 x i32>, i64)* @kernel, !13, !14, !4, !15}
!13 = !{i32 0, i32 0, i32 0}
!14 = !{i32 0, i32 1, i32 2}
!15 = !{i32 1, i32 -1, i32 2}
!16 = distinct !DISubprogram(name: "kernel", scope: null, file: !3, line: 6, type: !17, scopeLine: 7, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !26)
!17 = !DISubroutineType(types: !18)
!18 = !{null, !19, !21}
!19 = !DIDerivedType(tag: DW_TAG_typedef, name: "SurfaceIndex_buffer_t_int", file: !3, baseType: !20)
!20 = !DIBasicType(name: "SurfaceIndex", size: 32, encoding: DW_ATE_unsigned)
!21 = !DICompositeType(tag: DW_TAG_array_type, baseType: !22, size: 4096, elements: !23)
!22 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!23 = !{!24, !25}
!24 = !DISubrange(count: 8)
!25 = !DISubrange(count: 16)
!26 = !{!29, !33, !34, !35}
!29 = !DILocalVariable(name: "D5", scope: !16, file: !3, line: 18, type: !30)
!30 = !DICompositeType(tag: DW_TAG_array_type, baseType: !22, size: 128, flags: DIFlagVector, elements: !31)
!31 = !{!32}
!32 = !DISubrange(count: 4)
!33 = !DILocalVariable(name: "D9", scope: !16, file: !3, line: 23, type: !30)
!34 = !DILocalVariable(name: "D13", scope: !16, file: !3, line: 29, type: !30)
!35 = !DILocalVariable(name: "D15", scope: !16, file: !3, line: 31, type: !30)
!37 = !DILocation(line: 0, scope: !16)
