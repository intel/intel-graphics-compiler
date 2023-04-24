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

; COM: the kernel should have two intervals corresponding to subroutine
; COM: (since subroutines are copied between kernels and stack-calls)

; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_K1_dwarf.elf | FileCheck %s --check-prefix=CHECK_SUBROUTINE
; COM: this matches any subprogram, then search for Subroutine record
; CHECK_SUBROUTINE: DW_TAG_subprogram
; CHECK_SUBROUTINE: DW_AT_name : Subroutine
; CHECK_SUBROUTINE-NOT: DW_TAG_subprogram
; CHECK_SUBROUTINE: DW_AT_low_pc
; CHECK_SUBROUTINE-NEXT: DW_AT_high_pc

; COM: this matches any subprogram, then search for Subroutine record
; CHECK_SUBROUTINE: DW_TAG_subprogram
; CHECK_SUBROUTINE: DW_AT_name : Subroutine
; CHECK_SUBROUTINE: DW_AT_low_pc
; CHECK_SUBROUTINE-NEXT: DW_AT_high_pc

; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_K1_dwarf.elf | FileCheck %s --check-prefix=CHECK_KERNEL
; COM: this matches any subprogram, then search for K1 record
; CHECK_KERNEL: DW_TAG_subprogram
; CHECK_KERNEL: DW_AT_name : K1
; CHECK_KERNEL-NOT: DW_TAG_subprogram
; CHECK_KERNEL: DW_AT_low_pc
; CHECK_KERNEL-NEXT: DW_AT_high_pc

; RUN: oneapi-readelf --debug-dump dbginfo_%basename_t_K1_dwarf.elf | FileCheck %s --check-prefix=CHECK_STACK
; COM: this matches any subprogram, then search for S1 record
; CHECK_STACK: DW_TAG_subprogram
; CHECK_STACK: DW_AT_name : S1
; CHECK_STACK-NOT: DW_TAG_subprogram
; CHECK_STACK: DW_AT_low_pc
; CHECK_STACK-NEXT: DW_AT_high_pc

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind
declare void @llvm.genx.media.st.v8i32(i32, i32, i32, i32, i32, i32, <8 x i32>) #1

; Function Attrs: nounwind readonly
declare <8 x i32> @llvm.genx.oword.ld.v8i32(i32, i32, i32) #4

; Function Attrs: noinline nounwind
define internal spir_func void @Subr(<8 x i32> %0) unnamed_addr #0 !dbg !15 {
  tail call void @llvm.genx.media.st.v8i32(i32 0, i32 1, i32 0, i32 32, i32 0, i32 0, <8 x i32> %0), !dbg !18
  ret void
}

; Function Attrs: noinline nounwind
define internal spir_func void @S1(<8 x i32> %0) unnamed_addr #2 !dbg !20 !FuncArgSize !27 !FuncRetSize !28 {
  tail call spir_func void @Subr(<8 x i32> %0) #0, !dbg !21
  ret void
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @K1(i32 %0, i64 %privBase) local_unnamed_addr #3 !dbg !23 {
  %2 = tail call <8 x i32> @llvm.genx.oword.ld.v8i32(i32 0, i32 1, i32 0)
  tail call spir_func void @S1(<8 x i32> %2) #0, !dbg !25, !FuncArgSize !27, !FuncRetSize !28
  tail call spir_func void @Subr(<8 x i32> %2) #0, !dbg !26
  ret void
}
attributes #0 = { noinline nounwind }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind "CMStackCall" }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" }
attributes #4 = { nounwind readonly }

!llvm.module.flags = !{!0, !1}
!llvm.dbg.cu = !{!2}
!genx.kernels = !{!8}
!genx.kernel.internal = !{!13}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "the_file.cpp", directory: "/the_directory")
!4 = !{}
!5 = !{i32 0, i32 0}
!6 = !{i32 1, i32 2}
!7 = !{i16 6, i16 14}
!8 = !{void (i32, i64)* @K1, !"K1", !9, i32 0, !10, !11, !12, i32 0}
!9 = !{i32 2, i32 96}
!10 = !{i32 72, i32 64}
!11 = !{i32 0}
!12 = !{!"buffer_t read_write"}
!13 = !{void (i32, i64)* @K1, !5, !14, !4, !6}
!14 = !{i32 0, i32 1}
!15 = distinct !DISubprogram(name: "Subroutine", scope: null, file: !3, line: 3, type: !16, scopeLine: 3, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, templateParams: !4, retainedNodes: !4)
!16 = !DISubroutineType(types: !17)
!17 = !{null}
!18 = !DILocation(line: 4, column: 3, scope: !15)
!20 = distinct !DISubprogram(name: "S1", scope: null, file: !3, line: 6, type: !16, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !2, templateParams: !4, retainedNodes: !4)
!21 = !DILocation(line: 7, column: 10, scope: !20)
!23 = distinct !DISubprogram(name: "K1", scope: null, file: !3, line: 10, type: !16, scopeLine: 10, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagMainSubprogram, unit: !2, templateParams: !4, retainedNodes: !4)
!25 = !DILocation(line: 13, column: 3, scope: !23)
!26 = !DILocation(line: 14, column: 3, scope: !23)
!27 = !{i32 1}
!28 = !{i32 0}
