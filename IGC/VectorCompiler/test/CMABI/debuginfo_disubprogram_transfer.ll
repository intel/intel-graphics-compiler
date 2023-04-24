;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; here we basically check that verifier does not crash
; CHECK: @test

source_filename = "disubprogram_transfer.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; Function Attrs: noinline nounwind optnone
define spir_func void @test(<8 x i32> addrspace(4)* %0) #0 !dbg !10 {
  ret void, !dbg !12
}

attributes #0 = { noinline nounwind optnone "target-cpu"="Gen9" }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{}
!genx.kernel.internal = !{}
!llvm.dbg.cu = !{!5}
!llvm.debugify = !{!7, !8}
!llvm.module.flags = !{!9}

!0 = !{i32 0, i32 100000}
!1 = !{i32 1, i32 2}
!2 = !{i32 1, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = distinct !DICompileUnit(language: DW_LANG_C, file: !6, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !3)
!6 = !DIFile(filename: "disubprogram_transfer.ll", directory: "/")
!7 = !{i32 44}
!8 = !{i32 28}
!9 = !{i32 2, !"Debug Info Version", i32 3}
!10 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !6, line: 1, type: !11, scopeLine: 1, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !5, retainedNodes: !3)
!11 = !DISubroutineType(types: !3)
!12 = !DILocation(line: 44, column: 1, scope: !10)
