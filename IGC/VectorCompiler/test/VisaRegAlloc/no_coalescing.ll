;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -genx-dump-regalloc \
; RUN: -vc-disable-coalescing \
; RUN: -vc-fg-dump-prefix=%basename_t_ \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; COM: These checks are brittle and differ between LLVM versions.
; COM: Currently accounted for with {{(<pre-llvm11-val>|<llvm11-val>)}} regexp syntax.
; FIXME: Return unconditional checks once LLVM version is unified for IGC.

; RUN: FileCheck %s --input-file=%basename_t_M_.regalloc --check-prefix=CHECK_NOCOALESC
; CHECK_NOCOALESC: [t7] (4 bytes, length 13) arg1:[0,13)
; CHECK_NOCOALESC-NEXT: [t6] (4 bytes, length 7) arg:[0,7)
; CHECK_NOCOALESC-NEXT: [v32] (64 bytes, length {{(4|6)}}) :[5,{{(9|11)}})
; CHECK_NOCOALESC-NEXT: [v33] (64 bytes, length {{(4|2)}}) :[7,{{(11|9)}})
; CHECK_NOCOALESC-NEXT: [v34] (64 bytes, length 2) :[9,11)
; CHECK_NOCOALESC-NEXt: [v35] (64 bytes, length 2) :[11,13)
; CHECK_NOCOALESC: Register pressure (bytes):
; CHECK_NOCOALESC: Flag pressure (bytes):

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -genx-dump-regalloc \
; RUN: -vc-fg-dump-prefix=%basename_t_ \
; RUN: -finalizer-opts='-generateDebugInfo' -o /dev/null

; RUN: FileCheck %s --input-file=%basename_t_M_.regalloc --check-prefix=CHECK_COALESC
; CHECK_COALESC: [t7] (4 bytes, length 13) arg1:[0,13)
; CHECK_COALESC-NEXT: [t6] (4 bytes, length 7) arg:[0,7)
; CHECK_COALESC-NEXT: [v{{(33|32)}}] (64 bytes, length 6) :[5,11)
; CHECK_COALESC-NEXT: [v{{(32|33)}}] (64 bytes, length 4) :[7,11)
; CHECK_COALESC-NEXt: [v34] (64 bytes, length 2) :[11,13)
; CHECK_COALESC: Register pressure (bytes):
; CHECK_COALESC: Flag pressure (bytes):

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readonly
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32) #1

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>) #2

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test_kernel(i32 %0, i32 %1) local_unnamed_addr #0 {
  %3 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %4 = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 1)
  %5 = add <8 x i64> %4, %3
  %6 = add <8 x i64> %5, %4
  tail call void @llvm.genx.oword.st.v8i64(i32 %1, i32 0, <8 x i64> %6)
  ret void
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
!genx.kernels = !{!6}
!genx.kernel.internal = !{!10}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "kernel_genx.cpp", directory: "/the_directory/")
!4 = !{}
!5 = !{i32 0, i32 0}
!6 = !{void (i32, i32)* @test_kernel, !"test_kernel", !7, i32 0, !8, !5, !9, i32 0}
!7 = !{i32 2, i32 2}
!8 = !{i32 64, i32 68}
!9 = !{!"buffer_t", !"buffer_t"}
!10 = !{void (i32, i32)* @test_kernel, null, null, null, null}
