;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -simdcf-region -enable-simdcf-transform -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------
; This test checks that GenXSimdCFRegion generate
; correct if-then simd llvm-ir (based on ispc test)

; Function Attrs: nounwind
; CHECK: f_v
define dllexport spir_kernel void @f_v(i8* %RET, i64 %impl.arg.private.base) #2 {
allocas:
  %pow_const = call <16 x float> @llvm.genx.pow.v16f32.v16f32(<16 x float> <float 1.0, float 1.0, float 0.0, float 0.0, float 1.0, float 1.0, float 0.0, float 0.0, float 1.0, float 1.0, float 0.0, float 0.0, float 1.0, float 1.0, float 0.0, float 0.0>, <16 x float> <float 0.0, float 1.0, float 0.0, float 1.0, float 0.0, float 1.0, float 0.0, float 1.0, float 0.0, float 1.0, float 0.0, float 1.0, float 0.0, float 1.0, float 0.0, float 1.0>)
  %fcmp_result = fcmp olt <16 x float> %pow_const, <float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0>
  %any_check = call i1 @llvm.genx.all.v16i1(<16 x i1> %fcmp_result)
  br i1 %any_check, label %exit, label %if

; CHECK: [[AFTER_ALLOCAS:[A-z0-9.]*]]:
; CHECK: [[GOTO:%[A-z0-9.]*]] = call { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto
; CHECK: {{.*}} = extractvalue { <32 x i1>, <16 x i1>, i1 } [[GOTO]]
; CHECK: [[IF:[A-z0-9.]*]]: ; preds = %[[AFTER_ALLOCAS]]
if:                                           ; preds = %allocas
  %fdiv_res = fdiv <16 x float> %pow_const, <float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0>
  %fcmp_olt_res = fcmp olt <16 x float> %fdiv_res, <float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0, float 0.0>
  %if_result = or <16 x i1> %fcmp_result, %fcmp_olt_res
  %if_result.select = select <16 x i1> %fcmp_result,  <16 x i1> %if_result ,<16 x i1> zeroinitializer
  br label %exit

; CHECK: [[AFTERTHEN:[A-z0-9.]*]]: ; preds = %[[IF]], %[[AFTER_ALLOCAS]]
; CHECK: [[JOIN:%[A-z0-9.]*]] = call { <32 x i1>, i1 } @llvm.genx.simdcf.join
; CHECK: %{{.*}} = extractvalue { <32 x i1>, i1 } [[JOIN]]
; CHECK: [[EXIT:[A-z0-9.]*]]: ; preds = %[[AFTERTHEN]]
exit:                                         ; preds = %if, %allocas
  %logical_op_mem.0.in.i = phi <16 x i1> [ %if_result.select, %if ], [ %fcmp_result, %allocas ]
  %blend.i = select <16 x i1> %logical_op_mem.0.in.i, <16 x float> <float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0, float 1.0>, <16 x float> zeroinitializer
  %svm_st_ptrtoint = ptrtoint i8* %RET to i64
  call void @llvm.genx.svm.block.st.i64.v16f32(i64 %svm_st_ptrtoint, <16 x float> %blend.i)
  ret void
}

declare i1 @llvm.genx.all.v16i1(<16 x i1>) #0
declare <16 x float> @llvm.genx.pow.v16f32.v16f32(<16 x float>, <16 x float>) #0
declare void @llvm.genx.svm.block.st.i64.v16f32(i64, <16 x float>) #1

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }
attributes #2 = { nounwind "CMGenxMain" "oclrt"="1" }

!genx.kernels = !{!4}
!genx.kernel.internal = !{!10}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i8*, i64)* @f_v, !"f_v", !5, i32 0, !6, !7, !8, i32 0}
!5 = !{i32 0, i32 96}
!6 = !{i32 72, i32 64}
!7 = !{i32 0}
!8 = !{!"svmptr_t"}
!10 = !{void (i8*, i64)* @f_v, !0, !11, !2, !12}
!11 = !{i32 0, i32 1}
!12 = !{i32 255, i32 255}
!14 = !{i32 1}
