;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null \
; RUN: | FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK-DAG: .decl  [[DST:V[^ ]+]] v_type=G type=w num_elts=16
; CHECK: mov (M1, 8) [[DST]](0,0)<2> V{{[0-9]+}}(0,0)<1;1,0>

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;


declare <16 x i16> @llvm.genx.wrregioni.v16i16.v8i16.i16.v8i1(<16 x i16>, <8 x i16>, i32, i32, i32, i16, i32, <8 x i1>)

declare void @llvm.genx.svm.scatter.v8i1.v8i64.v16i16(<8 x i1>, i32, <8 x i64>, <16 x i16>)

define spir_kernel void @_ZTSZ4testIN4sycl3_V13ext6oneapi8bfloat16EEbNS1_5queueEiEUlvE_(i8 addrspace(4)* %_arg_Mem, i64 %impl.arg.private.base) #0 {
entry:
  %call1.i.i.i.esimd = tail call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> zeroinitializer)
  br label %for.body.i

for.body.i:                                       ; preds = %for.body.i, %entry
  %Vec.i.sroa.0.01 = phi <8 x i16> [ zeroinitializer, %entry ], [ %bf164, %for.body.i ]
  %call1.i14.i.i.esimd = tail call <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16> %Vec.i.sroa.0.01)
  %bf164 = tail call <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float> %call1.i14.i.i.esimd)
  br i1 undef, label %_ZZ4testIN4sycl3_V13ext6oneapi8bfloat16EEbNS1_5queueEiENKUlvE_clEv.exit, label %for.body.i

_ZZ4testIN4sycl3_V13ext6oneapi8bfloat16EEbNS1_5queueEiENKUlvE_clEv.exit: ; preds = %for.body.i
  %call5.i.i.esimd = tail call <16 x i16> @llvm.genx.wrregioni.v16i16.v8i16.i16.v8i1(<16 x i16> undef, <8 x i16> %bf164, i32 0, i32 8, i32 2, i16 0, i32 0, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  tail call void @llvm.genx.svm.scatter.v8i1.v8i64.v16i16(<8 x i1> zeroinitializer, i32 0, <8 x i64> zeroinitializer, <16 x i16> %call5.i.i.esimd)
  ret void
}

declare <8 x float> @llvm.vc.internal.cast.from.bf16.v8f32.v8i16(<8 x i16>)

declare <8 x i16> @llvm.vc.internal.cast.to.bf16.v8i16.v8f32(<8 x float>)

; uselistorder directives
uselistorder <8 x float> (<8 x i16>)* @llvm.vc.internal.cast.from.bf16.v8f32.v8i16, { 1, 0 }

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i8 addrspace(4)*, i64)* @_ZTSZ4testIN4sycl3_V13ext6oneapi8bfloat16EEbNS1_5queueEiEUlvE_, !"_ZTSZ4testIN4sycl3_V13ext6oneapi8bfloat16EEbNS1_5queueEiEUlvE_", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 96}
!2 = !{i32 136, i32 128}
!3 = !{i32 0}
!4 = !{!"svmptr_t", !""}
!5 = !{void (i8 addrspace(4)*, i64)* @_ZTSZ4testIN4sycl3_V13ext6oneapi8bfloat16EEbNS1_5queueEiEUlvE_, !6, !7, !8, !9}
!6 = !{i32 0, i32 0}
!7 = !{i32 0, i32 1}
!8 = !{}
!9 = !{i32 255, i32 255}
