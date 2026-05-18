;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s

;; Test that bfloat constants in the TRUE and FALSE operands of a select are
;; loaded as 16-bit integer immediates and then reinterpreted as bfloat.
;;
;; According to the HW spec, bfloat immediate values are not allowed, so VC
;; must put the constant into a register as an i16 immediate and bitcast it.
;; This tests the visitSelectInst path in ConstantLoadHelper (CI build
;; gfx-driver-ci-comp_igc-24512, VMIT-10341): for bfloat select, all 3
;; operands (condition, true value, false value) are checked, not just the
;; condition. The existing constant_bfloat16.ll test only covers the false
;; value (operand 2); this file covers operand 1 (true value) and both
;; operands 1+2 simultaneously.

declare <16 x bfloat> @llvm.genx.oword.ld.v16bf16(i32, i32, i32)
declare void @llvm.genx.oword.st.v16bf16(i32, i32, <16 x bfloat>)

; CHECK-LABEL: @select_true_const
; Bfloat constant 1.0 (0xR3F80 = i16 16256) in the TRUE operand (operand 1) of
; select must be loaded: constanti on i16, bitcast to bfloat, rdregion to splat.
; CHECK: [[CONST_T:%[^ ]+]] = call <1 x i16> @llvm.genx.constanti.v1i16(<1 x i16> <i16 16256>)
; CHECK: [[CAST_T:%[^ ]+]] = bitcast <1 x i16> [[CONST_T]] to <1 x bfloat>
; CHECK: [[SPLAT_T:%[^ ]+]] = call <16 x bfloat> @llvm.genx.rdregionf.v16bf16.v1bf16.i16(<1 x bfloat> [[CAST_T]], i32 0, i32 16, i32 0, i16 0, i32 undef)
; CHECK: %res = select <16 x i1> %pred, <16 x bfloat> [[SPLAT_T]], <16 x bfloat> %src
define dllexport spir_kernel void @select_true_const(i32 %buf) local_unnamed_addr #0 {
  %src  = call <16 x bfloat> @llvm.genx.oword.ld.v16bf16(i32 0, i32 %buf, i32 0)
  %src2 = call <16 x bfloat> @llvm.genx.oword.ld.v16bf16(i32 0, i32 %buf, i32 2)
  %pred = fcmp olt <16 x bfloat> %src, %src2
  %res  = select <16 x i1> %pred,
              <16 x bfloat> <bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80,
                              bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80,
                              bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80,
                              bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80>,
              <16 x bfloat> %src
  call void @llvm.genx.oword.st.v16bf16(i32 %buf, i32 0, <16 x bfloat> %res)
  ret void
}

; CHECK-LABEL: @select_both_const
; Both the TRUE operand (bfloat 1.0 splat) and FALSE operand (zeroinitializer)
; of the select are bfloat constants. Both must be loaded separately.
; CHECK: [[CONST_T2:%[^ ]+]] = call <1 x i16> @llvm.genx.constanti.v1i16(<1 x i16> <i16 16256>)
; CHECK: [[CAST_T2:%[^ ]+]] = bitcast <1 x i16> [[CONST_T2]] to <1 x bfloat>
; CHECK: [[SPLAT_T2:%[^ ]+]] = call <16 x bfloat> @llvm.genx.rdregionf.v16bf16.v1bf16.i16(<1 x bfloat> [[CAST_T2]], i32 0, i32 16, i32 0, i16 0, i32 undef)
; CHECK: [[CONST_F2:%[^ ]+]] = call <1 x i16> @llvm.genx.constanti.v1i16(<1 x i16> zeroinitializer)
; CHECK: [[CAST_F2:%[^ ]+]] = bitcast <1 x i16> [[CONST_F2]] to <1 x bfloat>
; CHECK: [[SPLAT_F2:%[^ ]+]] = call <16 x bfloat> @llvm.genx.rdregionf.v16bf16.v1bf16.i16(<1 x bfloat> [[CAST_F2]], i32 0, i32 16, i32 0, i16 0, i32 undef)
; CHECK: %res = select <16 x i1> %pred, <16 x bfloat> [[SPLAT_T2]], <16 x bfloat> [[SPLAT_F2]]
define dllexport spir_kernel void @select_both_const(i32 %buf) local_unnamed_addr #0 {
  %src1 = call <16 x bfloat> @llvm.genx.oword.ld.v16bf16(i32 0, i32 %buf, i32 0)
  %src2 = call <16 x bfloat> @llvm.genx.oword.ld.v16bf16(i32 0, i32 %buf, i32 2)
  %pred = fcmp olt <16 x bfloat> %src1, %src2
  %res  = select <16 x i1> %pred,
              <16 x bfloat> <bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80,
                              bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80,
                              bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80,
                              bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80, bfloat 0xR3F80>,
              <16 x bfloat> zeroinitializer
  call void @llvm.genx.oword.st.v16bf16(i32 %buf, i32 0, <16 x bfloat> %res)
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!0, !5}
!genx.kernel.internal = !{!6, !7}

!0 = !{void (i32)* @select_true_const, !"select_true_const", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 2}
!2 = !{i32 64}
!3 = !{i32 0}
!4 = !{!"buffer_t"}
!5 = !{void (i32)* @select_both_const, !"select_both_const", !1, i32 0, !2, !3, !4, i32 0}
!6 = !{void (i32)* @select_true_const, null, null, null, null}
!7 = !{void (i32)* @select_both_const, null, null, null, null}
