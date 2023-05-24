;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: llc %s -march=genx64 -mcpu=XeLP -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s --check-prefix=CHECK

target triple = "genx64-unknown-unknown"

declare void @llvm.genx.oword.st.v4i32(i32, i32, <4 x i32>)
declare void @llvm.debugtrap()

; CHECK: .decl [[REG:V[0-9]+]] v_type=G type=d num_elts=4 alias=<%cr0, 0>
; CHECK: test_trap
; CHECK: or (M1, 1) [[REG]](0,1)<1> [[REG]](0,1)<0;1,0> 0x20000000:d

define spir_kernel void @test_trap(<4 x i32> %arg) local_unnamed_addr #0 {
  %1 = shl <4 x i32> %arg, <i32 3, i32 3, i32 3, i32 3>
  %2 = add <4 x i32> %1, <i32 12, i32 12, i32 12, i32 12>
  tail call void @llvm.debugtrap()
  tail call void @llvm.genx.oword.st.v4i32(i32 1, i32 0, <4 x i32> %2)
  ret void
}


attributes #0 = { noinline nounwind "CMGenxMain" }

!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!0}
!opencl.used.optional.core.features = !{!0}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{}
!1 = !{i32 0}
!2 = !{i32 1, i32 1}
!3 = !{i16 6, i16 14}
!4 = !{void (<4 x i32>)* @test_trap, !"test_trap", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 1}
!6 = !{i32 64}
!7 = !{!"buffer_t"}
!8 = !{void (<4 x i32>)* @test_trap, null, null, null, null}
