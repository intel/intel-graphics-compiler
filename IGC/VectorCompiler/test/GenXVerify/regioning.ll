;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXVerify -genx-verify-terminate=no -genx-verify-all-fatal=1 -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s 2>&1 | \
; RUN:     FileCheck --check-prefixes=CHECK %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Beware: some of those declarations may be semantically invalid. Here we check IR validity, so this is normal.
declare i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <2 x i32> @llvm.genx.rdregioni.v2i32.v4i64.i16(<4 x i64>, i32, i32, i32, i16, i32)
declare <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.v2i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, <2 x i1>)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v2i32.i16.i1(<4 x i32>, <2 x i32>, i32, i32, i32, i16, i32, i1)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v8i32.i16.i1(<4 x i32>, <8 x i32>, i32, i32, i32, i16, i32, i1)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32>, i32, i32, i32, i32, i16, i32, i1)
declare i32 @llvm.genx.wrregioni.i32.i32.i16.i1(i32, i32, i32, i32, i32, i16, i32, i1)

declare <16 x i1> @llvm.genx.wrpredpredregion.v16i1.v16i1(<16 x i1>, <16 x i1>, i32, <16 x i1>)

declare <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1>, i32)
declare <32 x i1> @llvm.genx.rdpredregion.v32i1.v32i1(<32 x i1>, i32)
declare <32 x i1> @llvm.genx.wrpredregion.v32i1.v16i1(<32 x i1>, <16 x i1>, i32)
declare <32 x i1> @llvm.genx.wrpredregion.v32i1.v32i1(<32 x i1>, <32 x i1>, i32)

declare <16 x i32> @llvm.genx.wrconstregion.v16i32.v1i32.i16.i1(<16 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @regioning() {
  %non_const_i32 = add i32 42, 0
  %non_const_i16 = add i16 42, 0
  %non_const_v1i32 = insertelement <1 x i32> undef, i32 42, i32 0

  ; ------------ *predregion* -------------
  %chk_wrpredpredregion_subvector_is_from_cmp = call <16 x i1> @llvm.genx.wrpredpredregion.v16i1.v16i1(<16 x i1> undef, <16 x i1> undef, i32 0, <16 x i1> undef)
 ; CHECK: warning: {{.+}} %chk_wrpredpredregion_subvector_is_from_cmp = {{.+}} subvector to write must be the direct result of a cmp instruction

  %chk_rdpredregion_off_const = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> undef, i32 %non_const_i32);
; CHECK: warning: {{.+}} %chk_rdpredregion_off_const {{.+}} offset in elements must be a constant integer

  %chk_rdpredregion_off = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> undef, i32 17);
; CHECK: warning: {{.+}} %chk_rdpredregion_off {{.+}} offset in elements must be multiple of the number of elements of returned vector

  %chk_rdpredregion_num_ret = call <32 x i1> @llvm.genx.rdpredregion.v32i1.v32i1(<32 x i1> undef, i32 16);
; CHECK: warning: {{.+}} %chk_rdpredregion_num_ret {{.+}} returned vector must be 4, 8 or 16

  %chk_wrpredregion_off = call <32 x i1> @llvm.genx.wrpredregion.v32i1.v16i1(<32 x i1> undef, <16 x i1> undef, i32 17)
; CHECK: warning: {{.+}} %chk_wrpredregion_off {{.+}} offset in elements must be multiple of the number of elements of subvector to write

  %chk_wrpredregion_num_subvect = call <32 x i1> @llvm.genx.wrpredregion.v32i1.v32i1(<32 x i1> undef, <32 x i1> undef, i32 16)
; CHECK: warning: {{.+}} %chk_wrpredregion_num_subvect {{.+}} subvector to write must be 4, 8 or 16

  ; ------------ rdregion* and common rdregion/wrregion* -------------
  %chk_ret_vec_type = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> undef, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: warning: {{.+}} %chk_ret_vec_type = {{.+}} return type must be a vector

  %chk_width1 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> undef, i32 0, i32 3, i32 0, i16 0, i32 0)
  %chk_width2 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> undef, i32 0, i32 %non_const_i32, i32 0, i16 0, i32 0)
; CHECK: warning: {{.+}} %chk_width1 = {{.+}} width must divide the total size evenly
; CHECK: warning: {{.+}} %chk_width2 = {{.+}} width must be a constant int

  %chk_vstride_const = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> undef, i32 %non_const_i32, i32 1, i32 0, i16 0, i32 undef)
; CHECK: warning: {{.+}} %chk_vstride_const = {{.+}} vertical stride must be a constant int
  %chk_stride_const = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> undef, i32 0, i32 1, i32 %non_const_i32, i16 0, i32 undef)
; CHECK: warning: {{.+}} %chk_stride_const = {{.+}} horizontal stride must be a constant int

  %chk_ret_el_type = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i64.i16(<4 x i64> undef, i32 2, i32 2, i32 1, i16 0, i32 undef)
; CHECK: warning: {{.+}} %chk_ret_el_type = {{.+}} return type must be a vector with the same element type as the input vector

  ; ! TODO:spec review: no currently defined restrictions on offset.
  %chk_offset = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> undef, i32 2, i32 2, i32 1, i16 100500, i32 undef)

  ; ! TODO:spec review: no currently defined restrictions on parent width.
  %chk_parent_width1 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> undef, i32 2, i32 2, i32 1, i16 %non_const_i16, i32 -100)
  %chk_parent_width2 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> undef, i32 2, i32 2, i32 1, i16 %non_const_i16, i32 0)
  %chk_parent_width3 = call <2 x i32> @llvm.genx.rdregioni.v2i32.v4i32.i16(<4 x i32> undef, i32 2, i32 2, i32 1, i16 %non_const_i16, i32 %non_const_i32)
; CHECK: warning: {{.+}} %chk_parent_width1 = {{.+}} [[WS:parent width when not ignored \(offset arg is not a constant\) must be a valid constant integer with the value > 0]]
; CHECK: warning: {{.+}} %chk_parent_width2 = {{.+}} [[WS]]
; CHECK: warning: {{.+}} %chk_parent_width3 = {{.+}} [[WS]]

  ; ------------ wrregion*-specific -------------
  %chk_scalar_subregion_width1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.i32.i16.i1(<4 x i32> undef, i32 undef, i32 0, i32 2, i32 0, i16 0, i32 undef, i1 1)
; CHECK: warning: {{.+}} %chk_scalar_subregion_width1 = {{.+}} subregion to write may be a scalar if the number of elements in subregion is 1

  %chk_mask_vec_size = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.v2i1(<4 x i32> undef, <4 x i32> undef, i32 0, i32 1, i32 0, i16 0, i32 undef, <2 x i1> <i1 1, i1 1>);
; CHECK: warning: {{.+}} %chk_mask_vec_size = {{.+}} arg7 mask can be a vector of booleans, exactly as wide as the arg1 subvector, such that an element of the subvector is written into its place in the vector only if the corresponding element of the mask is true.

; ! TODO:spec review: spec requires i1 mask to be 1 not explicitly specifying if 0 value is expected or not.
  %chk_scalar_mask_is_1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> undef, <4 x i32> undef, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 0)
; CHECK: warning: {{.+}} %chk_scalar_mask_is_1 = {{.+}} arg7 mask can be a single i1 constant with value 1, meaning that the wrregion is unconditional.

  %chk_subregion_fits = call <4 x i32> @llvm.genx.wrregioni.v4i32.v8i32.i16.i1(<4 x i32> undef, <8 x i32> undef, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 0)
; CHECK: warning: {{.+}} %chk_subregion_fits = {{.+}} arg1 subvector must be no larger than arg0 vector

; ! TODO:spec review: spec doesn't allow non-vector dstsrc operand, whereas this case is found in the wild.
  %chk_dstsrc_vec = call i32 @llvm.genx.wrregioni.i32.i32.i16.i1(i32 undef, i32 undef, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 0)
; CHECK: warning: {{.+}} %chk_dstsrc_vec = {{.+}} destination-source (arg0) must be a fixed vector

  %chk_constant_off = call <16 x i32> @llvm.genx.wrconstregion.v16i32.v1i32.i16.i1(<16 x i32> undef, <1 x i32> <i32 42>, i32 1, i32 1, i32 1, i16 %non_const_i16, i32 undef, i1 true)
; CHECK: warning: {{.+}} %chk_constant_off = {{.+}} offset must be a constant

  %chk_constant_subv = call <16 x i32> @llvm.genx.wrconstregion.v16i32.v1i32.i16.i1(<16 x i32> undef, <1 x i32> %non_const_v1i32, i32 1, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: warning: {{.+}} %chk_constant_subv = {{.+}} subvector to write must be a constant

  ret void
}
