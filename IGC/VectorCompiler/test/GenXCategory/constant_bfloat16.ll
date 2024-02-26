;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s

;; Test legalization of bfloat16 constant

declare <16 x bfloat> @llvm.genx.oword.ld.v16bf16(i32, i32, i32)
declare void @llvm.genx.oword.st.v16bf16(i32, i32, <16 x bfloat>)

; CHECK-LABEL: @const_zero
; CHECK: [[CONST:%[^ ]+]] = call <1 x i16> @llvm.genx.constanti.v1i16(<1 x i16> zeroinitializer)
; CHECK: [[CAST:%[^ ]+]] = bitcast <1 x i16> [[CONST]] to <1 x bfloat>
; CHECK: [[SPLAT:%[^ ]+]] = call <16 x bfloat> @llvm.genx.rdregionf.v16bf16.v1bf16.i16(<1 x bfloat> [[CAST]], i32 0, i32 16, i32 0, i16 0, i32 undef)
; CHECK: %res = fadd <16 x bfloat> %src, [[SPLAT]]
define dllexport spir_kernel void @const_zero(i32 %buf) local_unnamed_addr #0 {
  %src = call <16 x bfloat> @llvm.genx.oword.ld.v16bf16(i32 0, i32 %buf, i32 0)
  %res = fadd <16 x bfloat> %src, zeroinitializer
  call void @llvm.genx.oword.st.v16bf16(i32 %buf, i32 0, <16 x bfloat> %res)
  ret void
}

; CHECK-LABEL: @const_one
; CHECK: [[CONST:%[^ ]+]] = call <1 x i16> @llvm.genx.constanti.v1i16(<1 x i16> <i16 16256>)
; CHECK: [[CAST:%[^ ]+]] = bitcast <1 x i16> [[CONST]] to <1 x bfloat>
; CHECK: [[SPLAT:%[^ ]+]] = call <16 x bfloat> @llvm.genx.rdregionf.v16bf16.v1bf16.i16(<1 x bfloat> [[CAST]], i32 0, i32 16, i32 0, i16 0, i32 undef)
; CHECK: %res = fadd <16 x bfloat> %src, [[SPLAT]]
define dllexport spir_kernel void @const_one(i32 %buf) local_unnamed_addr #0 {
  %src = call <16 x bfloat> @llvm.genx.oword.ld.v16bf16(i32 0, i32 %buf, i32 0)
  %res = fadd <16 x bfloat> %src, <bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80>
  call void @llvm.genx.oword.st.v16bf16(i32 %buf, i32 0, <16 x bfloat> %res)
  ret void
}

; CHECK-LABEL: @const_vector
; CHECK: [[SCALAR:%[^ ]+]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> <i32 -1049607840>)
; CHECK: [[SPLAT:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v1i32.i16(<1 x i32> [[SCALAR]], i32 0, i32 8, i32 0, i16 0, i32 undef)
; CHECK: [[INS0:%[^ ]+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[SPLAT]], <1 x i32> <i32 -1051705024>, i32 1, i32 1, i32 1, i16 24, i32 undef, i1 true)
; CHECK: [[INS1:%[^ ]+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[INS0]], <1 x i32> <i32 -1053802208>, i32 1, i32 1, i32 1, i16 20, i32 undef, i1 true)
; CHECK: [[INS2:%[^ ]+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[INS1]], <1 x i32> <i32 -1055899392>, i32 1, i32 1, i32 1, i16 16, i32 undef, i1 true)
; CHECK: [[INS3:%[^ ]+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[INS2]], <1 x i32> <i32 -1059045184>, i32 1, i32 1, i32 1, i16 12, i32 undef, i1 true)
; CHECK: [[INS4:%[^ ]+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[INS3]], <1 x i32> <i32 -1063239552>, i32 1, i32 1, i32 1, i16 8, i32 undef, i1 true)
; CHECK: [[INS5:%[^ ]+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[INS4]], <1 x i32> <i32 -1069531136>, i32 1, i32 1, i32 1, i16 4, i32 undef, i1 true)
; CHECK: [[INS6:%[^ ]+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[INS5]], <1 x i32> <i32 -1082130432>, i32 1, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: [[CAST:%[^ ]+]] = bitcast <8 x i32> [[INS6]] to <16 x bfloat>
; CHECK: %res = fadd <16 x bfloat> %src, [[CAST]]
define dllexport spir_kernel void @const_vector(i32 %buf) local_unnamed_addr #0 {
  %src = call <16 x bfloat> @llvm.genx.oword.ld.v16bf16(i32 0, i32 %buf, i32 0)
  %res = fadd <16 x bfloat> %src, <bfloat 0xR0000, bfloat 0xRBF80, bfloat 0xR4000, bfloat 0xRC040, bfloat 0xR4080, bfloat 0xRC0A0, bfloat 0xR40C0, bfloat 0xRC0E0, bfloat 0xR4100, bfloat 0xRC110, bfloat 0xR4120, bfloat 0xRC130, bfloat 0xR4140, bfloat 0xRC150, bfloat 0xR4160, bfloat 0xRC170>
  call void @llvm.genx.oword.st.v16bf16(i32 %buf, i32 0, <16 x bfloat> %res)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!0, !5, !6}
!genx.kernel.internal = !{!7, !8, !9}

!0 = !{void (i32)* @const_zero, !"const_zero", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 2}
!2 = !{i32 64}
!3 = !{i32 0}
!4 = !{!"buffer_t"}
!5 = !{void (i32)* @const_one, !"const_one", !1, i32 0, !2, !3, !4, i32 0}
!6 = !{void (i32)* @const_vector, !"const_vector", !1, i32 0, !2, !3, !4, i32 0}
!7 = !{void (i32)* @const_zero, null, null, null, null}
!8 = !{void (i32)* @const_one, null, null, null, null}
!9 = !{void (i32)* @const_vector, null, null, null, null}
