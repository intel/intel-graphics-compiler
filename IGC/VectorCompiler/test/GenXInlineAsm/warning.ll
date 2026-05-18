;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe2 -o /dev/null 2>&1 | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe2 -o /dev/null 2>&1 | FileCheck %s
; CHECK: warning: GenXCisaBuilder failed for: < %call = call i32 asm "mov (M1, 1) $0 $1", "=a,i"(i32 %rdregioni) #{{[0-9]+}}, !srcloc !{{[0-9]+}}, !genx.inlasm.constraints.info !{{[0-9]+}}>: "mov (M1, 1) $0 $1" immediate constraint in inline assembly was satisfied to value

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @test(i32 %0, i32 %1, i64 %privBase) #2 {
  %vec = tail call <8 x i32> @llvm.genx.oword.ld.v8i32(i32 0, i32 %0, i32 0)
  %3 = extractelement <8 x i32> %vec, i32 0
  %4 = call i32 asm "mov (M1, 1) $0 $1", "=a,i"(i32 %3) #1, !srcloc !42
  ret void
}

declare <8 x i32> @llvm.genx.oword.ld.v8i32(i32, i32, i32)

attributes #2 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}
!llvm.module.flags = !{!21, !22}

!0 = !{i32 0, i32 0}
!2 = !{}
!4 = !{void (i32, i32, i64)* @test, !"test", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 2, i32 2, i32 96}
!6 = !{i32 72, i32 80, i32 64}
!7 = !{!"buffer_t read_write", !"buffer_t read_write"}
!8 = !{void (i32, i32, i64)* @test, !9, !10, !2, !10}
!9 = !{i32 0, i32 0, i32 0}
!10 = !{i32 0, i32 1, i32 2}
!21 = !{i32 2, !"Dwarf Version", i32 4}
!22 = !{i32 2, !"Debug Info Version", i32 3}
!24 = !{null}
!42 = !{ i32 3 }

