;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXFuncBaling -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x float> @foo___vyfvyf(<16 x float>, <16 x float>, <1 x i16>)
declare <16 x float> @bar___vyfvyf(<16 x float>, <16 x float>, <1 x i16>)

declare <16 x float> @get_arg()
declare <1 x i16> @get_arg1()
declare i64 @llvm.genx.faddr.p0f_v16f32v16f32v16f32v16i1f(<16 x float> (<16 x float>, <16 x float>, <1 x i16>)*)
declare <16 x float> @llvm.genx.rdregionf.v16f32.v1f32.i16(<1 x float>, i32, i32, i32, i16, i32)
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32>, i32, i32, i32, i16, i32)
declare <16 x float> @llvm.genx.svm.block.ld.v16f32.i64(i64)
declare void @llvm.genx.svm.block.st.i64.v16f32(i64, <16 x float>)
declare <1 x float> @llvm.genx.svm.gather.v1f32.v1i1.v1i64(<1 x i1>, i32, <1 x i64>, <1 x float>)
declare <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32>, <1 x i32> , i32, i32, i32, i16, i32, i1)
declare <2 x i32> @llvm.genx.wrregioni.v2i32.v2i32.i16.i1(<2 x i32>, <2 x i32>, i32, i32, i32, i16, i32, i1)

; CHECK: call i64 @llvm.genx.faddr.p0f_v16f32v16f32v16f32v16i1f
; CHECK: call i64 @llvm.genx.faddr.p0f_v16f32v16f32v16f32v16i1f
; CHECK: call spir_func <16 x float>
define dllexport void @f_f(float* noalias %RET, float* noalias %aFOO, i64 %privBase) #4 { allocas:
  %svm_ld_ptrtoint = ptrtoint float* %aFOO to i64
  %aFOO_load_ptr2int_2void2728_masked_load29 = call <16 x float> @llvm.genx.svm.block.ld.v16f32.i64(i64 %svm_ld_ptrtoint)
  %0 = bitcast i64 %svm_ld_ptrtoint to <1 x i64>
  %1 = call <1 x float> @llvm.genx.svm.gather.v1f32.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %0, <1 x float> undef)
  %2 = bitcast <1 x float> %1 to float
  %sub_aFOO_load8_offset_load_ = fadd float %2, -1.000000e+00
  %sub_aFOO_load8_offset_load__broadcast111 = bitcast float %sub_aFOO_load8_offset_load_ to <1 x float>
  %sub_aFOO_load8_offset_load__broadcast11 = call <16 x float> @llvm.genx.rdregionf.v16f32.v1f32.i16(<1 x float> %sub_aFOO_load8_offset_load__broadcast111, i32 0, i32 16, i32 0, i16 0, i32 undef)
  %equal_aFOO_load12_offset_load_ = fcmp oeq float %2, 1.000000e+00
  %faddr = call i64 @llvm.genx.faddr.p0f_v16f32v16f32v16f32v16i1f(<16 x float> (<16 x float>, <16 x float>, <1 x i16>)* @foo___vyfvyf)
  %faddr.cast = bitcast i64 %faddr to <2 x i32>
  %3 = call <2 x i32> @llvm.genx.wrregioni.v2i32.v2i32.i16.i1(<2 x i32> undef, <2 x i32> %faddr.cast, i32 0, i32 2, i32 1, i16 0, i32 undef, i1 true)
  %faddr4 = call i64 @llvm.genx.faddr.p0f_v16f32v16f32v16f32v16i1f(<16 x float> (<16 x float>, <16 x float>, <1 x i16>)* @bar___vyfvyf)
  %faddr4.cast = bitcast i64 %faddr4 to <2 x i32>
  %4 = call <2 x i32> @llvm.genx.wrregioni.v2i32.v2i32.i16.i1(<2 x i32> undef, <2 x i32> %faddr4.cast, i32 0, i32 2, i32 1, i16 0, i32 undef, i1 true)
  %func.03.LoSplit = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %3, i32 0, i32 1, i32 2, i16 0, i32 undef)
  %func.03.HiSplit = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %3, i32 0, i32 1, i32 2, i16 4, i32 undef)
  %func.03.LoSplit10 = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %4, i32 0, i32 1, i32 2, i16 0, i32 undef)
  %func.03.HiSplit11 = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %4, i32 0, i32 1, i32 2, i16 4, i32 undef)
  %sel.lo = select i1 %equal_aFOO_load12_offset_load_, <1 x i32> %func.03.LoSplit, <1 x i32> %func.03.LoSplit10
  %sel.hi = select i1 %equal_aFOO_load12_offset_load_, <1 x i32> %func.03.HiSplit, <1 x i32> %func.03.HiSplit11
  %int_emu.select.partial_join = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> undef, <1 x i32> %sel.lo, i32 0, i32 1, i32 2, i16 0, i32 undef, i1 true)
  %int_emu.select.joined = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> %int_emu.select.partial_join, <1 x i32> %sel.hi, i32 0, i32 1, i32 2, i16 4, i32 undef, i1 true)
  %int_emu.select. = bitcast <2 x i32> %int_emu.select.joined to <1 x i64>
  %5 = bitcast <1 x i64> %int_emu.select. to i64
  %6 = inttoptr i64 %5 to <16 x float> (<16 x float>, <16 x float>, <1 x i16>)*
  %arg1 = call <16 x float> @get_arg()
  %arg2 = call <1 x i16> @get_arg1()
  %calltmp = call spir_func <16 x float> %6(<16 x float> %arg1, <16 x float> %arg1, <1 x i16> %arg2)
  %svm_st_ptrtoint = ptrtoint float* %RET to i64
  call void @llvm.genx.svm.block.st.i64.v16f32(i64 %svm_st_ptrtoint, <16 x float> %calltmp)
  ret void
}
