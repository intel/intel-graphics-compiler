;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that ExtractConstantData does not crash when encountering a ConstantExpr
; with an aggregate (vector) result type, where getAggregateElement returns null.

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeHPG \
; RUN: -vc-skip-ocl-runtime-info \
; RUN: -vc-enable-dbginfo-dumps -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeHPG \
; RUN: -vc-skip-ocl-runtime-info \
; RUN: -vc-enable-dbginfo-dumps -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: llvm-dwarfdump -debug-info dbginfo_%basename_t_test_kernel_dwarf.elf | FileCheck %s

; The main check is that compilation doesn't crash. Additionally verify
; the variable is present in debug info.
; CHECK: DW_TAG_variable
; CHECK: DW_AT_name ("vec_const")

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

@glb = internal addrspace(1) global i64 0

; Function Attrs: nounwind readonly
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32) #1

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>) #2

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test_kernel(i32 %0, i32 %1) local_unnamed_addr #0 !dbg !12 {
  ; A ConstantExpr bitcast producing a vector type — getAggregateElement
  ; returns null for this, which used to crash ExtractConstantData.
  call void @llvm.dbg.value(metadata <2 x i32> bitcast (i64 ptrtoint (i64 addrspace(1)* @glb to i64) to <2 x i32>), metadata !17, metadata !DIExpression()), !dbg !20
  %3 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0), !dbg !20
  tail call void @llvm.genx.oword.st.v8i64(i32 %1, i32 0, <8 x i64> %3), !dbg !21
  ret void, !dbg !22
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #3

attributes #0 = { noinline nounwind "CMGenxMain" }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }
attributes #3 = { nofree nosync nounwind readnone speculatable willreturn }

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
!genx.kernel.internal = !{!23}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "test.cpp", directory: "/test/")
!4 = !{}
!5 = !{i32 0, i32 0}
!6 = !{i32 1, i32 2}
!7 = !{i16 6, i16 14}
!8 = !{void (i32, i32)* @test_kernel, !"test_kernel", !9, i32 0, !10, !5, !11, i32 0}
!9 = !{i32 2, i32 2}
!10 = !{i32 64, i32 68}
!11 = !{!"buffer_t", !"buffer_t"}
!12 = distinct !DISubprogram(name: "test_kernel", scope: null, file: !3, line: 6, type: !13, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !16)
!13 = !DISubroutineType(types: !14)
!14 = !{null}
!15 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!16 = !{!17}
!17 = !DILocalVariable(name: "vec_const", scope: !12, file: !3, line: 7, type: !18)
!18 = !DICompositeType(tag: DW_TAG_array_type, baseType: !15, size: 64, elements: !19)
!19 = !{!DISubrange(count: 2)}
!20 = !DILocation(line: 7, column: 5, scope: !12)
!21 = !DILocation(line: 8, column: 5, scope: !12)
!22 = !DILocation(line: 9, column: 1, scope: !12)
!23 = !{void (i32, i32)* @test_kernel, null, null, null, null}
