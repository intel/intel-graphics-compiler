;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=XeHPG -finalizer-opts='-dumpvisa -dumpcommonisa' -o /dev/null
; RUN: cat test_merge_slm.visaasm | FileCheck %s --check-prefix=CHECK

target triple = "genx64-unknown-unknown"

declare <4 x i64> @llvm.genx.lsc.load.merge.slm.v4i64.v4i1.v4i32(<4 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <4 x i32>, i32, <4 x i64>)

declare void @llvm.genx.oword.st.v4i64(i32, i32, <4 x i64>)

; Here checked sequence:
;     mov (M1, 1) V34(0,0)<1> 0x0:q
;     mov (M1, 4) V35(0,0)<1> V34(0,0)<0;1,0> ; <- is it really needed?
;     (P1) lsc_load.slm (M1, 4)  V35:d64  flat[V37+0xc]:a32
; If constant propagation change - need fix this test
; CHECK: test_merge_slm
; CHECK: mov {{.*}}[[REG:V[0-9]+]]{{.*}}[[REG2:V[0-9]+]]
; CHECK-DAG: lsc_load.slm {{[^V]+}}[[REG]]

define spir_kernel void @test_merge_slm(<4 x i32> %arg) local_unnamed_addr #0 {
  %1 = shl <4 x i32> %arg, <i32 3, i32 3, i32 3, i32 3>
  %2 = add <4 x i32> %1, <i32 12, i32 12, i32 12, i32 12>
  %3 = tail call <4 x i64> @llvm.genx.lsc.load.merge.slm.v4i64.v4i1.v4i32(<4 x i1> <i1 true, i1 false, i1 false, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, <4 x i32> %2, i32 0, <4 x i64> zeroinitializer)
  tail call void @llvm.genx.oword.st.v4i64(i32 1, i32 0, <4 x i64> %3)
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
!4 = !{void (<4 x i32>)* @test_merge_slm, !"test_merge_slm", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 1}
!6 = !{i32 64}
!7 = !{!"buffer_t"}
!8 = !{void (<4 x i32>)* @test_merge_slm, null, null, null, null}
