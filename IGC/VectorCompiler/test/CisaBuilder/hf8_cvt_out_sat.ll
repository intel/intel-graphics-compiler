;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3 -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3 -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s


; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK-DAG: .decl [[SRC:V[^ ]+]] v_type=G type=hf num_elts=16 alias
; CHECK-DAG: .decl [[DST:V[^ ]+]] v_type=G type=b num_elts=16 alias
; CHECK: fcvt.sat (M1_NM, 16) [[DST]](0,0)<1> [[SRC]](0,0)<1;1,0>



; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <16 x half> @llvm.genx.oword.ld.v16f16(i32, i32, i32)
declare <16 x i8> @llvm.genx.hf8.cvt.v16i8.v16f16(<16 x half>)
declare <16 x i8> @llvm.genx.sstrunc.sat.v16i8.v16i8(<16 x i8>)
declare void @llvm.genx.oword.st.v16i8(i32, i32, <16 x i8>)

define dllexport spir_kernel void @hf8_cvtKernel_out_sat(i32 %0, i32 %1) local_unnamed_addr #0 {
  %vec = tail call <16 x half> @llvm.genx.oword.ld.v16f16(i32 0, i32 %0, i32 0)
  %upd_vec = tail call <16 x i8> @llvm.genx.hf8.cvt.v16i8.v16f16(<16 x half> %vec)
  %sat = tail call <16 x i8> @llvm.genx.sstrunc.sat.v16i8.v16i8(<16 x i8> %upd_vec)
  tail call void @llvm.genx.oword.st.v16i8(i32 %1, i32 0, <16 x i8> %sat)
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
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i16 6, i16 14}
!4 = !{void (i32, i32)* @hf8_cvtKernel_out_sat, !"hf8_cvtKernel_out_sat", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @hf8_cvtKernel_out_sat, null, null, null, null}
