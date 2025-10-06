;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --CheckInstrTypes --igc-update-instrtypes-on-run -inputocl --neo -platformpvc -igc-emit-visa -simd-mode 16 %s 2>&1 | FileCheck %s

; CHECK: Constraints for inline assembly cannot be validated
define spir_kernel void @test() {
  %1 = call i16 @llvm.genx.GenISA.simdLaneId()
  %2 = zext i16 %1 to i32
  %3 = call i32 asm "xor (M1, 16) $0(0,0)<1> $0(0,0)<1;1,0> $1(0,0)<0;1,0>", "=rw,rw.u,0"(i32 %2, i32 7) #2
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!1}


!1 = !{void ()* @test, !2}
!2 = !{!3, !4}
!3 = !{!"function_type", i32 0}
!4 = !{!"sub_group_size", i32 16}
