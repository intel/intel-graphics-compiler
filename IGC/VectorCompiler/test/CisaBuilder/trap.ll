;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: llc %s -march=genx64 -mcpu=XeLP -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s --check-prefix=CHECK
; RUN: llc %s -march=genx64 -mcpu=XeHPG -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s --check-prefix=CHECK-LSC

target triple = "genx64-unknown-unknown"

declare void @llvm.genx.oword.st.v4i32(i32, i32, <4 x i32>)
declare void @llvm.trap()

; CHECK: test_trap
; CHECK: mov (M1, 8) [[PAYLOAD:V[0-9]+]](0,0)<1> %r0(0,0)<1;1,0>
; CHECK: raw_sends_eot.7.1.0.0 (M1, 1)  0x0:ud 0x2000010:ud [[PAYLOAD]].0 %null.0 %null.0

; CHECK-LSC: test_trap
; CHECK-LSC: mov (M1, 8) [[PAYLOAD:V[0-9]+]](0,0)<1> %r0(0,0)<1;1,0>
; CHECK-LSC: raw_sends_eot.3.1.0.0 (M1, 1)  0x0:ud 0x2000010:ud [[PAYLOAD]].0 %null.0 %null.0

define spir_kernel void @test_trap(<4 x i32> %arg) local_unnamed_addr #0 {
  %1 = shl <4 x i32> %arg, <i32 3, i32 3, i32 3, i32 3>
  %2 = add <4 x i32> %1, <i32 12, i32 12, i32 12, i32 12>
  tail call void @llvm.trap()
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
