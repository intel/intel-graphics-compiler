
;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=XeLPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=XeLPG -vc-skip-ocl-runtime-info -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; CHECK: .decl [[REG_0:V[^ ]+]] v_type=G type=ud num_elts=4
; CHECK: .decl [[REG_1:V[^ ]+]] v_type=G type=ud num_elts=4

; CHECK: cmp.lt (M1, 4) P1 [[REG_0]](0,0)<1;1,0> 0x2b:ud
; CHECK: cmp.lt (M2, 4) P1 [[REG_1]](0,0)<1;1,0> 0x2b:ud
; CHECK: cmp.gt (M3, 4) P1 [[REG_0]](0,0)<1;1,0> 0x4:ud
; CHECK: cmp.gt (M4, 4) P1 [[REG_1]](0,0)<1;1,0> 0x4:ud

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

declare !genx_intrinsic_id !30 void @llvm.genx.svm.scatter.v16i1.v16i64.v16i64(<16 x i1>, i32, <16 x i64>, <16 x i64>) #1
declare !genx_intrinsic_id !31 <16 x i1> @llvm.genx.wrpredregion.v16i1.v4i1(<16 x i1>, <4 x i1>, i32) #2

define dllexport spir_kernel void @kernel(<16 x i64> %src, <16 x i64> %data, <4 x i32> %A, <4 x i32> %B) local_unnamed_addr #3 {
  %cmp.0 = icmp ule <4 x i32> %A, <i32 42, i32 42, i32 42, i32 42>
  %cmp.1 = icmp ule <4 x i32> %B, <i32 42, i32 42, i32 42, i32 42>
  %cmp.2 = icmp uge <4 x i32> %A, <i32 5, i32 5, i32 5, i32 5>
  %cmp.3 = icmp uge <4 x i32> %B, <i32 5, i32 5, i32 5, i32 5>
  %wrpred.0 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v4i1(<16 x i1> undef, <4 x i1> %cmp.0, i32 0)
  %wrpred.1 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v4i1(<16 x i1> %wrpred.0, <4 x i1> %cmp.1, i32 4)
  %wrpred.2 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v4i1(<16 x i1> %wrpred.1, <4 x i1> %cmp.2, i32 8)
  %wrpred.3 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v4i1(<16 x i1> %wrpred.2, <4 x i1> %cmp.3, i32 12)
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v16i64(<16 x i1> %wrpred.3, i32 0, <16 x i64> %src, <16 x i64> %data)
  ret void
}

attributes #1 = { nounwind }
attributes #2 = { nounwind readnone }
attributes #3 = { mustprogress noinline nounwind willreturn "CMGenxMain" "oclrt"="1" }

!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{}
!1 = !{i32 0}
!2 = !{i32 1, i32 1}
!3 = !{i16 6, i16 14}
!4 = !{void (<16 x i64>, <16 x i64>, <4 x i32>, <4 x i32>)* @kernel, !"kernel", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0, i32 0, i32 0, i32 0}
!6 = !{i32 128, i32 256, i32 384, i32 512}
!7 = !{!""}
!8 = !{void (<16 x i64>, <16 x i64>, <4 x i32>, <4 x i32>)* @kernel, null, null, null, null}

!30 = !{i32 11069}
!31 = !{i32 11178}
