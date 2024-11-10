;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefix=CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefix=CHECK-OPAQUE-PTRS

declare void @llvm.genx.svm.scatter.v16i1.v16i64.v16f32(<16 x i1>, i32, <16 x i64>, <16 x float>) #0

; CHECK-TYPED-PTRS: define internal spir_func void @test(<16 x float>*
; CHECK-OPAQUE-PTRS: define internal spir_func void @test(ptr
define internal spir_func void @test(<16 x float>* noalias nocapture %a) #3 {
  %a_load_offset = getelementptr <16 x float>, <16 x float>* %a, i64 5
  %ptr_to_int.i.i = ptrtoint <16 x float>* %a_load_offset to i64
  %base.i.i = insertelement <16 x i64> undef, i64 %ptr_to_int.i.i, i64 0
  %shuffle.i.i = shufflevector <16 x i64> %base.i.i, <16 x i64> undef, <16 x i32> zeroinitializer
  %new_offsets.i.i = add <16 x i64> %shuffle.i.i, <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28, i64 32, i64 36, i64 40, i64 44, i64 48, i64 52, i64 56, i64 60>
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v16f32(<16 x i1> <i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1, i1 1>, i32 0, <16 x i64> %new_offsets.i.i, <16 x float> zeroinitializer)
  ret void
}

define dllexport spir_kernel void @kernel() #4 {
  %x = alloca [10 x <16 x float>], align 64
  %x_offset = getelementptr inbounds [10 x <16 x float>], [10 x <16 x float>]* %x, i64 0, i64 0
  call spir_func void @test(<16 x float>* noalias nocapture nonnull %x_offset) #5
  ret void
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind readonly }
attributes #3 = { noinline nounwind "CMStackCall" }
attributes #4 = { nounwind "CMGenxMain" "oclrt"="1" }
attributes #5 = { noinline nounwind }

