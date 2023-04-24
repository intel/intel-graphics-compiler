;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check subroutine after stack-call argument definition
; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps \
; RUN: -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_kernel_dwarf.elf | FileCheck %s

; CHECK: DW_AT_name           : arg_2
; CHECK-NEXT: DW_AT_decl_file : 1
; CHECK-NEXT: DW_AT_decl_line : 6
; CHECK-NEXT: DW_AT_type      : <{{0x[0-9a-f]+}}>
; CHECK-NEXT: DW_AT_location

; ModuleID = 'Deserialized LLVM Module'
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: noinline nounwind
define internal spir_func <4 x i32> @subroutine(<4 x i32> %0) #0{
  call void @llvm.dbg.value(metadata <4 x i32> %0, metadata !24, metadata !DIExpression()), !dbg !26
  %2 = add <4 x i32> %0, <i32 1, i32 1, i32 1, i32 1>
  ret <4 x i32> %2
}

; Function Attrs: noinline nounwind
define internal spir_func <4 x i32> @stack_call(<4 x i32> %0) #2 !FuncArgSize !51 !FuncRetSize !52 {
  %2 = add <4 x i32> %0, <i32 1, i32 1, i32 1, i32 1>
  %3 = call spir_func <4 x i32> @subroutine(<4 x i32> %2) #0
  ret <4 x i32> %3
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @kernel(i32 %0, <4 x i32> %1, i64 %privBase) #3 {
  %3 = call spir_func <4 x i32> @stack_call(<4 x i32> %1) #0, !FuncArgSize !51, !FuncRetSize !52
  %4 = sub <4 x i32> %3, %1
  call void @llvm.genx.oword.st.v4i32(i32 1, i32 0, <4 x i32> %4)
  ret void
}

declare void @llvm.dbg.declare(metadata, metadata, metadata) #1
declare !intel_reqd_sub_group_size !50 dllexport spir_kernel void @0(i32, <4 x i32>) #3
declare void @llvm.genx.oword.st.v4i32(i32, i32, <4 x i32>) #4
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind }
attributes #1 = { nounwind readnone speculatable willreturn }
attributes #2 = { noinline nounwind "CMStackCall" }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" }
attributes #4 = { nounwind }

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
!genx.kernel.internal = !{!12}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "src.cm", directory: "./")
!4 = !{}
!5 = !{i32 0, i32 0}
!6 = !{i32 1, i32 2}
!7 = !{i16 6, i16 14}
!8 = !{void (i32, <4 x i32>, i64)* @kernel, !"kernel", !9, i32 0, !10, !5, !11, i32 0}
!9 = !{i32 2, i32 0, i32 96}
!10 = !{i32 72, i32 80, i32 64}
!11 = !{!"buffer_t read_write", !""}
!12 = !{void (i32, <4 x i32>, i64)* @kernel, !13, !14, !4, !15}
!13 = !{i32 0, i32 0, i32 0}
!14 = !{i32 0, i32 1, i32 2}
!15 = !{i32 1, i32 -1, i32 2}
!16 = distinct !DISubprogram(name: "subroutine", linkageName: "subroutine", scope: null, file: !3, line: 6, type: !17, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, templateParams: !4, retainedNodes: !23)
!17 = !DISubroutineType(types: !18)
!18 = !{!19, !19}
!19 = !DICompositeType(tag: DW_TAG_array_type, baseType: !20, size: 128, flags: DIFlagVector, elements: !21)
!20 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!21 = !{!22}
!22 = !DISubrange(count: 4)
!23 = !{!24}
!24 = !DILocalVariable(name: "arg_2", arg: 1, scope: !16, file: !3, line: 6, type: !19)
!26 = !DILocation(line: 0, scope: !16)
!50 = !{i32 1}
!51 = !{i32 1}
!52 = !{i32 1}
