;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-enable-dbginfo-dumps -vc-dbginfo-dumps-name-override=%basename_t \
; RUN: -finalizer-opts='-generateDebugInfo -noschedule' -o /dev/null

; RUN: llvm-dwarfdump -a dbginfo_%basename_t_test_kernel_dwarf.elf | FileCheck %s

; CHECK: Address Line Column File ISA Discriminator Flags
; CHECK:           42      0    1   0           0   is_stmt
; CHECK-NEXT:       1     18    1   0           0   is_stmt
; CHECK-NEXT:     666     10    1   0           0   is_stmt
; CHECK-NEXT:     777     20    1   0           0   is_stmt
; CHECK-NEXT:     555     30    1   0           0   is_stmt
; CHECK-NEXT:    9999     40    1   0           0   is_stmt

; ModuleID = 'Deserialized SPIRV Module'
target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readonly
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32) #1

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>) #2

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test_kernel(i32 %0, i32 %1) local_unnamed_addr #0 !dbg !12 {
  %3 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0), !dbg !18
  %4 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 512), !dbg !19
  %5 = add <8 x i64> %4, %3, !dbg !20
  tail call void @llvm.genx.oword.st.v8i64(i32 %1, i32 0, <8 x i64> %5), !dbg !21
  ret void, !dbg !22
}

attributes #0 = { noinline nounwind "CMGenxMain" }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }
attributes #3 = { noinline nounwind readnone }
attributes #4 = { noinline norecurse nounwind readnone }
attributes #5 = { nounwind readnone }

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
!genx.kernel.internal = !{!24}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "kernel_genx.cpp", directory: "/the_directory/")
!4 = !{}
!5 = !{i32 0, i32 0}
!6 = !{i32 1, i32 2}
!7 = !{i16 6, i16 14}
!8 = !{void (i32, i32)* @test_kernel, !"test_kernel", !9, i32 0, !10, !5, !11, i32 0}
!9 = !{i32 2, i32 2}
!10 = !{i32 64, i32 68}
!11 = !{!"buffer_t", !"buffer_t"}
!12 = distinct !DISubprogram(name: "test_kernel", scope: null, file: !3, line: 42, type: !13, scopeLine: 9, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !4)
!13 = !DISubroutineType(types: !14)
!14 = !{null}
!18 = !DILocation(line: 1, column: 18, scope: !12)
!19 = !DILocation(line: 666, column: 10, scope: !12)
!20 = !DILocation(line: 777, column: 20, scope: !12)
!21 = !DILocation(line: 555, column: 30, scope: !12)
!22 = !DILocation(line: 9999, column: 40, scope: !12)
!23 = !{i32 7687}
!24 = !{void (i32, i32)* @test_kernel, null, null, null, null}
