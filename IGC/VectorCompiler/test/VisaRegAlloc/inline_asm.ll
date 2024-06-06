;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=XeHPG -vc-skip-ocl-runtime-info \
; RUN: -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s

; CHECK-NOT: (P{{[0-9]+}}) add (M1, 8) [[OUT:V[0-9]+]](0,0)<1> [[OUT]](0,0)<1;1,0> V{{[0-9]+}}(0,0)<1;1,0>

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(i32 %_arg_VL, i8 addrspace(4)* %_arg_PA, i8 addrspace(4)* %_arg_PB, i8 addrspace(4)* %_arg_PC, <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i32> %impl.arg.llvm.genx.local.size, i64 %impl.arg.private.base) #0 {
  %1 = bitcast i8 addrspace(4)* %_arg_PA to <8 x i32> addrspace(4)*
  %2 = load <8 x i32>, <8 x i32> addrspace(4)* %1, align 4
  %3 = bitcast i8 addrspace(4)* %_arg_PB to <8 x i32> addrspace(4)*
  %4 = load <8 x i32>, <8 x i32> addrspace(4)* %3, align 4
  %5 = tail call <8 x i16> asm "mov (M1, 8) $0 0x1010101:v", "=r"()
  %6 = tail call <8 x i32> asm "{\0A.decl P1 v_type=P num_elts=8\0Amov (M1, 8) $0 0x1:ud\0Asetp (M1, 8) P1 $3\0A(P1) add (M1, 8) $0 $1 $2\0A}", "=r,r,r,r"(<8 x i32> %2, <8 x i32> %4, <8 x i16> %5)
  %7 = mul <8 x i32> %6, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %8 = bitcast i8 addrspace(4)* %_arg_PC to <8 x i32> addrspace(4)*
  store <8 x i32> %7, <8 x i32> addrspace(4)* %8, align 4
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32, i8 addrspace(4)*, i8 addrspace(4)*, i8 addrspace(4)*, <3 x i16>, <3 x i32>, i64)* @test, !"test", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0, i32 0, i32 0, i32 24, i32 8, i32 96}
!2 = !{i32 88, i32 96, i32 104, i32 112, i32 32, i32 64, i32 80}
!3 = !{i32 0, i32 0, i32 0, i32 0}
!4 = !{!"", !"svmptr_t", !"svmptr_t", !"svmptr_t"}
!5 = !{void (i32, i8 addrspace(4)*, i8 addrspace(4)*, i8 addrspace(4)*, <3 x i16>, <3 x i32>, i64)* @test, !6, !7, !8, !9}
!6 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6}
!8 = !{}
!9 = !{i32 -1, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255}
