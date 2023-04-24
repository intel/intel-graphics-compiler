;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: llvm-dwarfdump -debug-info dbginfo_%basename_t_main_kernel_dwarf.elf | FileCheck %s

; CHECK: DW_TAG_compile_unit
; CHECK: DW_AT_name ("stack_example.cpp")
; CHECK: DW_AT_low_pc
; CHECK: DW_AT_high_pc

; CHECK: DW_TAG_subprogram
; CHECK: DW_AT_name ("main_kernel")
; CHECK: DW_AT_low_pc
; CHECK: DW_AT_high_pc

; CHECK: DW_TAG_subprogram
; CHECK: DW_AT_name ("callee")
; CHECK: DW_AT_low_pc
; CHECK: DW_AT_high_pc

; ------------------------------------------------
; VC_asm1cc98cdb81cc9c41_optimized.ll
; ------------------------------------------------
; ModuleID = 'Deserialized SPIRV Module'
target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

; Function Attrs: noinline nounwind
define internal spir_func void @_Z6callee15cm_surfaceindexu2CMvr8_x(<8 x i64> %0) unnamed_addr #0 !FuncArgSize !26 !FuncRetSize !27 {
  tail call void @llvm.genx.oword.st.v8i64(i32 3, i32 0, <8 x i64> %0), !dbg !13
  ret void, !dbg !17
}

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>) #1

; Function Attrs: noinline nounwind
define dllexport void @main_kernel(i32 %0, i32 %1, i32 %2, i64 %privBase) local_unnamed_addr #2 !dbg !18 {
  %4 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 1, i32 0), !dbg !19
  %5 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 2, i32 0), !dbg !20
  %6 = add <8 x i64> %5, %4, !dbg !21
  tail call spir_func void @_Z6callee15cm_surfaceindexu2CMvr8_x(<8 x i64> %6) #9, !dbg !22, !FuncArgSize !26, !FuncRetSize !27
  ret void, !dbg !23
}

; Function Attrs: nounwind readonly
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32) #3

attributes #0 = { noinline nounwind "CMStackCall" }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind "CMGenxMain" "oclrt"="1" }
attributes #3 = { nounwind readonly }

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
!genx.kernel.internal = !{!25}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "stack_example.cpp", directory: "/the_directory")
!4 = !{}
!5 = !{i32 0, i32 0}
!6 = !{i32 1, i32 2}
!7 = !{i16 6, i16 14}
!8 = !{void (i32, i32, i32, i64)* @main_kernel, !"main_kernel", !9, i32 0, !10, !11, !12, i32 0}
!9 = !{i32 2, i32 2, i32 2, i32 96}
!10 = !{i32 136, i32 144, i32 152, i32 128}
!11 = !{i32 0, i32 0, i32 0}
!12 = !{!"buffer_t read_write", !"buffer_t read_write", !"buffer_t read_write"}
!13 = !DILocation(line: 7, column: 5, scope: !14)
!14 = distinct !DISubprogram(name: "callee", scope: null, file: !3, line: 5, type: !15, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, templateParams: !4, retainedNodes: !4)
!15 = !DISubroutineType(types: !16)
!16 = !{null}
!17 = !DILocation(line: 8, column: 1, scope: !14)
!18 = distinct !DISubprogram(name: "main_kernel", scope: null, file: !3, line: 10, type: !15, scopeLine: 14, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !4)
!19 = !DILocation(line: 19, column: 5, scope: !18)
!20 = !DILocation(line: 20, column: 5, scope: !18)
!21 = !DILocation(line: 22, column: 13, scope: !18)
!22 = !DILocation(line: 23, column: 5, scope: !18)
!23 = !DILocation(line: 24, column: 1, scope: !18)
!24 = !{i32 7713}
!25 = !{void (i32, i32, i32, i64)* @main_kernel, null, null, null, null}
!26 = !{i32 2}
!27 = !{i32 0}