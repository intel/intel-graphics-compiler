;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; ------------------------------------------------
; VC_asmf09c314a1705dfcb_optimized.ll
; ------------------------------------------------
; ModuleID = 'Deserialized SPIRV Module'
target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_vector_add_dwarf.elf | FileCheck %s
; RUN: llvm-dwarfdump dbginfo_%basename_t_vector_add_dwarf.elf | FileCheck %s --check-prefix DWARFDUMP

; CHECK: DW_TAG_variable
; DWARFDUMP:  DW_AT_name        ("offset")
; CHECK-NEXT: DW_AT_name        : offset
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; DWARFDUMP:  DW_AT_type        ({{0x[0-9a-f]+}} "unsigned int")
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_location    : 0x[[OFF_LOC:[0-9a-f]+]] (location list)


; CHECK: DW_TAG_variable
; DWARFDUMP:  DW_AT_name        ("ivector1")
; CHECK-NEXT: DW_AT_name        : ivector1
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; DWARFDUMP:  DW_AT_type        ({{0x[0-9a-f]+}} "int[8]")
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_location    : 0x[[IVEC1_LOC:[0-9a-f]+]] (location list)

; CHECK: DW_TAG_variable
; DWARFDUMP:  DW_AT_name        ("ivector2")
; CHECK-NEXT: DW_AT_name        : ivector2
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; DWARFDUMP:  DW_AT_type        ({{0x[0-9a-f]+}} "int[8]")
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_location    : 0x[[IVEC2_LOC:[0-9a-f]+]] (location list)

; CHECK: DW_TAG_variable
; DWARFDUMP:  DW_AT_name        ("ovector")
; CHECK-NEXT: DW_AT_name        : ovector
; CHECK-NEXT: DW_AT_decl_file
; CHECK-NEXT: DW_AT_decl_line
; DWARFDUMP:  DW_AT_type        ({{0x[0-9a-f]+}} "int[8]")
; CHECK-NEXT: DW_AT_type
; CHECK-NEXT: DW_AT_location    : 0x[[OVEC_LOC:[0-9a-f]+]] (location list)

; CHECK-DAG: [[OFF_LOC]] {{[^(]+}}(DW_OP_reg[[#]] (r[[#]]); DW_OP_{{lit|const1u: }}[[#]]; DW_OP_const1u: 32; DW_OP_INTEL_push_bit_piece_stack; DW_OP_constu: 6; DW_OP_shl; DW_OP_stack_value)
; FIXME: Once LLVM 11+ is enabled for all platforms, make ivector* location list checks explicit:
; {{[^(]+}}(DW_OP_reg[[#]] (r[[#]]); DW_OP_const1u: 0; DW_OP_const1u: 64; DW_OP_INTEL_push_bit_piece_stack)
; CHECK-DAG: [[IVEC1_LOC]] {{[^(]+}}(DW_OP_reg[[#]] (r[[#]]){{.*}})
; CHECK-DAG: [[IVEC2_LOC]] {{[^(]+}}(DW_OP_reg[[#]] (r[[#]]){{.*}})
; CHECK-DAG: [[OVEC_LOC]]  {{[^(]+}}(DW_OP_reg[[#]] (r[[#]]))


; Function Attrs: noinline nounwind
define dllexport spir_kernel void @vector_add(i32 %0, i32 %1, i32 %2) #0 !dbg !13 {
  %4 = alloca <8 x i32>, align 64
  %5 = alloca <8 x i32>, align 64
  call void @llvm.dbg.value(metadata i32 %0, metadata !18, metadata !DIExpression()), !dbg !30
  call void @llvm.dbg.value(metadata i32 %1, metadata !19, metadata !DIExpression()), !dbg !30
  call void @llvm.dbg.value(metadata i32 %2, metadata !20, metadata !DIExpression()), !dbg !30
  %6 = call i32 @llvm.genx.group.id.x(), !dbg !31
  call void @llvm.dbg.value(metadata i32 %6, metadata !28, metadata !DIExpression(DW_OP_constu, 6, DW_OP_shl, DW_OP_stack_value)), !dbg !30
  %7 = shl i32 %6, 2, !dbg !32
  %8 = and i32 %7, 268435452, !dbg !32
  %9 = call <8 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 %0, i32 %8), !dbg !32
  call void @llvm.dbg.value(metadata <8 x i32>* %4, metadata !21, metadata !DIExpression(DW_OP_deref)), !dbg !30
  store <8 x i32> %9, <8 x i32>* %4, !dbg !32
  %10 = call <8 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 %1, i32 %8), !dbg !33
  call void @llvm.dbg.value(metadata <8 x i32>* %5, metadata !26, metadata !DIExpression(DW_OP_deref)), !dbg !30
  store <8 x i32> %10, <8 x i32>* %5, !dbg !33
  %11 = load <8 x i32>, <8 x i32>* %4, align 64, !dbg !34
  call void @llvm.dbg.value(metadata <8 x i32> %11, metadata !21, metadata !DIExpression()), !dbg !30
  %12 = load <8 x i32>, <8 x i32>* %5, align 64, !dbg !35
  call void @llvm.dbg.value(metadata <8 x i32> %12, metadata !26, metadata !DIExpression()), !dbg !30
  %13 = add <8 x i32> %11, %12, !dbg !36
  call void @llvm.dbg.value(metadata <8 x i32> %13, metadata !27, metadata !DIExpression()), !dbg !30
  call void @llvm.genx.oword.st.v16i32(i32 %2, i32 %8, <8 x i32> %13), !dbg !37
  ret void, !dbg !38
}

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone
declare i32 @llvm.genx.group.id.x() #2

; Function Attrs: nounwind readonly
declare <8 x i32> @llvm.genx.oword.ld.v16i32(i32, i32, i32) #3

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v16i32(i32, i32, <8 x i32>) #4

attributes #0 = { noinline nounwind "CMGenxMain" }
attributes #1 = { nounwind readnone speculatable willreturn }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind readonly }
attributes #4 = { nounwind }
attributes #5 = { noinline nounwind "CMFloatControl"="0" }
attributes #6 = { noinline nounwind "CMFloatControl"="48" }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!opencl.enable.FP_CONTRACT = !{}
!spirv.Source = !{!5}
!opencl.spir.version = !{!6}
!opencl.ocl.version = !{!5}
!opencl.used.extensions = !{!4}
!opencl.used.optional.core.features = !{!4}
!spirv.Generator = !{!7}
!genx.kernels = !{!8}
!VC.Debug.Enable = !{}
!genx.kernel.internal = !{!39}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "the_file.cpp", directory: "/the_directory")
!4 = !{}
!5 = !{i32 0, i32 0}
!6 = !{i32 1, i32 2}
!7 = !{i16 6, i16 14}
!8 = !{void (i32, i32, i32)* @vector_add, !"vector_add", !9, i32 0, !10, !11, !12, i32 0}
!9 = !{i32 2, i32 2, i32 2}
!10 = !{i32 32, i32 36, i32 40}
!11 = !{i32 0, i32 0, i32 0}
!12 = !{!"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write"}
!13 = distinct !DISubprogram(name: "vector_add", scope: null, file: !3, line: 17, type: !14, scopeLine: 28, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !17)
!14 = !DISubroutineType(types: !15)
!15 = !{null, !16, !16, !16}
!16 = !DIBasicType(name: "SurfaceIndex", size: 32, encoding: DW_ATE_unsigned)
!17 = !{!18, !19, !20, !21, !26, !27, !28}
!18 = !DILocalVariable(name: "isurface1", arg: 1, scope: !13, file: !3, line: 23, type: !16)
!19 = !DILocalVariable(name: "isurface2", arg: 2, scope: !13, file: !3, line: 24, type: !16)
!20 = !DILocalVariable(name: "osurface", arg: 3, scope: !13, file: !3, line: 25, type: !16)
!21 = !DILocalVariable(name: "ivector1", scope: !13, file: !3, line: 31, type: !22)
!22 = !DICompositeType(tag: DW_TAG_array_type, baseType: !23, size: 256, flags: DIFlagVector, elements: !24)
!23 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!24 = !{!25}
!25 = !DISubrange(count: 8)
!26 = !DILocalVariable(name: "ivector2", scope: !13, file: !3, line: 32, type: !22)
!27 = !DILocalVariable(name: "ovector", scope: !13, file: !3, line: 33, type: !22)
!28 = !DILocalVariable(name: "offset", scope: !13, file: !3, line: 36, type: !29)
!29 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!30 = !DILocation(line: 0, scope: !13)
!31 = !DILocation(line: 36, column: 47, scope: !13)
!32 = !DILocation(line: 39, column: 5, scope: !13)
!33 = !DILocation(line: 40, column: 5, scope: !13)
!34 = !DILocation(line: 42, column: 15, scope: !13)
!35 = !DILocation(line: 42, column: 26, scope: !13)
!36 = !DILocation(line: 42, column: 24, scope: !13)
!37 = !DILocation(line: 46, column: 5, scope: !13)
!38 = !DILocation(line: 47, column: 1, scope: !13)
!39 = !{void (i32, i32, i32)* @vector_add, null, null, null, null}
