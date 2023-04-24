;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrologEpilogInsertion -dce -mattr=+ocl_runtime -march=genx64 \
; RUN: -mcpu=Gen9 -S -vc-arg-reg-size=32 -vc-ret-reg-size=12 < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <8 x float> @llvm.genx.svm.block.ld.v8f32.i64(i64)
declare void @llvm.genx.svm.block.st.i64.v8f32(i64, <8 x float>)
declare <8 x float> @llvm.genx.rdregionf.v8f32.v1f32.i16(<1 x float>, i32, i32, i32, i16, i32)
declare <1 x float> @llvm.genx.svm.gather.v1f32.v1i1.v1i64(<1 x i1>, i32, <1 x i64>, <1 x float>)
declare <96 x float> @llvm.genx.wrregionf.v96f32.v8f32.i16.i1(<96 x float>, <8 x float>, i32, i32, i32, i16, i32, i1)
declare <8 x float> @llvm.genx.write.predef.reg.v8f32.v96f32(i32, <96 x float>)
declare <256 x float> @llvm.genx.read.predef.reg.v256f32.v256f32(i32, <256 x float>)
declare <8 x float> @llvm.genx.rdregionf.v8f32.v256f32.i16(<256 x float>, i32, i32, i32, i16, i32)
declare <256 x float> @llvm.genx.wrregionf.v256f32.v8f32.i16.i1(<256 x float>, <8 x float>, i32, i32, i32, i16, i32, i1)
declare <8 x float> @llvm.genx.write.predef.reg.v8f32.v256f32(i32, <256 x float>)
declare <8 x float> @llvm.genx.read.predef.reg.v8f32.v8f32(i32, <8 x float>)
declare <8 x float> @llvm.genx.rdregionf.v8f32.v8f32.i16(<8 x float>, i32, i32, i32, i16, i32)
declare float @llvm.genx.rdregionf.f32.v8f32.i16(<8 x float>, i32, i32, i32, i16, i32)
declare <8 x float> @llvm.genx.wrregionf.v8f32.f32.i16.i1(<8 x float>, float, i32, i32, i32, i16, i32, i1)
declare i32 @llvm.genx.get.hwid()

define {float, <8 x float>} @foo___vyfvyf(<8 x float> %a, <8 x float> %b) #0 {
  ; CHECK-LABEL: foo___vyfvyf
  ; CHECK: %[[arg0_f:[^ ]+]] = call <256 x float> @llvm.genx.read.predef.reg.v256f32.v256f32(i32 8, <256 x float> undef)
  ; CHECK: %[[arg0rdr_f:[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v256f32.i16(<256 x float> %[[arg0_f]], i32 0, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK: %[[arg1_f:[^ ]+]] = call <256 x float> @llvm.genx.read.predef.reg.v256f32.v256f32(i32 8, <256 x float> undef)
  ; CHECK: %[[arg1rdr_f:[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v256f32.i16(<256 x float> %[[arg1_f]], i32 0, i32 8, i32 1, i16 32, i32 undef)
  ; CHECK: %[[add_f:[^ ]+]] = fadd <8 x float> %[[arg0rdr_f]], %[[arg1rdr_f]]
  %add_a_load_b_load = fadd <8 x float> %a, %b
  ; CHECK: %[[ret0_f:[^ ]+]] = call float @llvm.genx.rdregionf.f32.v8f32.i16(<8 x float> %[[add_f]], i32 0, i32 8, i32 1, i16 4, i32 undef)
  %ret0 = call float @llvm.genx.rdregionf.f32.v8f32.i16(<8 x float> %add_a_load_b_load, i32 0, i32 8, i32 1, i16 4, i32 undef)
  ; CHECK: %[[newret_f:[^ ]+]] = insertvalue { float, <8 x float> } undef, float %[[ret0_f]], 0
  %newret = insertvalue {float, <8 x float>} undef, float %ret0, 0
  ; CHECK: %[[newret1_f:[^ ]+]] = insertvalue { float, <8 x float> } %[[newret_f]], <8 x float> %[[add_f]], 1
  %newret1 = insertvalue {float, <8 x float>} %newret, <8 x float> %add_a_load_b_load, 1
  ; COM: Unlike IGC calling conv and old VC approach which are focused on insertvalue
  ; COM: instructions handling (a try to find insertvalue instructions in the code or
  ; COM: failure otherwise), new approach must be more robust and reliable as it
  ; COM: generates extractvalue instructions.
  ; COM: Structures are passed in ARG/RET in packed form as a vector of bytes.
  ; CHECK: %[[extract1:[^ ]+]] = extractvalue { float, <8 x float> } %[[newret1_f]], 0
  ; CHECK: %[[bitcast1:[^ ]+]] = bitcast float %[[extract1]] to <4 x i8>
  ; CHECK: %[[ret_read:[^ ]+]] = call <384 x i8> @llvm.genx.read.predef.reg.v384i8.v384i8(i32 9, <384 x i8> undef)
  ; CHECK: %[[ret_wrr:[^ ]+]] = call <384 x i8> @llvm.genx.wrregioni.v384i8.v4i8.i16.i1(<384 x i8> %[[ret_read]], <4 x i8> %[[bitcast1]], i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: call <4 x i8> @llvm.genx.write.predef.reg.v4i8.v384i8(i32 9, <384 x i8> %[[ret_wrr]])
  ; CHECK: %[[extract2:[^ ]+]] = extractvalue { float, <8 x float> } %[[newret1_f]], 1
  ; CHECK: %[[bitcast2:[^ ]+]] = bitcast <8 x float> %[[extract2]] to <32 x i8>
  ; CHECK: %[[ret_read2:[^ ]+]] = call <384 x i8> @llvm.genx.read.predef.reg.v384i8.v384i8(i32 9, <384 x i8> undef)
  ; CHECK: %[[ret_wrr2:[^ ]+]] = call <384 x i8> @llvm.genx.wrregioni.v384i8.v32i8.i16.i1(<384 x i8> %[[ret_read2]], <32 x i8> %[[bitcast2]], i32 0, i32 32, i32 1, i16 4, i32 undef, i1 true)
  ; CHECK: call <32 x i8> @llvm.genx.write.predef.reg.v32i8.v384i8(i32 9, <384 x i8> %[[ret_wrr2]])
  ; CHECK: ret { float, <8 x float> } %[[newret1_f]]
  ret {float, <8 x float>} %newret1
}

define void @f_f(float* %RET, float* %aFOO, i64 %privBase) #1 {
  ; CHECK-LABEL: f_f
  ; CHECK: %[[hwid:[^ ]+]] = call i32 @llvm.vc.internal.logical.thread.id()
  ; CHECK: %[[hwid_mul:[^ ]+]] = mul i32 %[[hwid]], 8192
  ; CHECK: %[[hwid_ext:[^ ]+]] = zext i32 %[[hwid_mul]] to i64
  ; CHECK: %[[sp:[^ ]+]] = add i64 %privBase, %[[hwid_ext]]
  ; CHECK: %[[spwrr:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[sp]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %[[spwrr]])
  %svm_ld_ptrtoint = ptrtoint float* %aFOO to i64
  %aFOO_load_ptr2int_2void2122_masked_load23 = call <8 x float> @llvm.genx.svm.block.ld.v8f32.i64(i64 %svm_ld_ptrtoint)
  %1 = bitcast i64 %svm_ld_ptrtoint to <1 x i64>
  %2 = call <1 x float> @llvm.genx.svm.gather.v1f32.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %1, <1 x float> undef)
  %3 = bitcast <1 x float> %2 to float
  %sub_aFOO_load8_offset_load_ = fadd float %3, -1.000000e+00
  %sub_aFOO_load8_offset_load__broadcast111 = bitcast float %sub_aFOO_load8_offset_load_ to <1 x float>
  %sub_aFOO_load8_offset_load__broadcast11 = call <8 x float> @llvm.genx.rdregionf.v8f32.v1f32.i16(<1 x float> %sub_aFOO_load8_offset_load__broadcast111, i32 0, i32 8, i32 0, i16 0, i32 undef)
  ; CHECK: %[[arg0:[^ ]+]] = call <256 x float> @llvm.genx.read.predef.reg.v256f32.v256f32(i32 8, <256 x float> undef)
  ; CHECK: %[[arg0wrr:[^ ]+]] = call <256 x float> @llvm.genx.wrregionf.v256f32.v8f32.i16.i1(<256 x float> %[[arg0]], <8 x float> %aFOO_load_ptr2int_2void2122_masked_load23, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: call <8 x float> @llvm.genx.write.predef.reg.v8f32.v256f32(i32 8, <256 x float> %[[arg0wrr]])
  ; CHECK: %[[arg1:[^ ]+]] = call <256 x float> @llvm.genx.read.predef.reg.v256f32.v256f32(i32 8, <256 x float> undef)
  ; CHECK: %[[arg1wrr:[^ ]+]] = call <256 x float> @llvm.genx.wrregionf.v256f32.v8f32.i16.i1(<256 x float> %[[arg1]], <8 x float> %sub_aFOO_load8_offset_load__broadcast11, i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK: call <8 x float> @llvm.genx.write.predef.reg.v8f32.v256f32(i32 8, <256 x float> %[[arg1wrr]])
  %calltmp = call spir_func {float, <8 x float>} @foo___vyfvyf(<8 x float> %aFOO_load_ptr2int_2void2122_masked_load23, <8 x float> %sub_aFOO_load8_offset_load__broadcast11)
  ; CHECK: %[[call:[^ ]+]] = call spir_func { float, <8 x float> } @foo___vyfvyf(<8 x float> %aFOO_load_ptr2int_2void2122_masked_load23, <8 x float> %sub_aFOO_load8_offset_load__broadcast11)
  ; CHECK: %[[ret_read1:[^ ]+]] = call <384 x i8> @llvm.genx.read.predef.reg.v384i8.v384i8(i32 9, <384 x i8> undef)
  ; CHECK: %[[ret_rdr1:[^ ]+]] = call <4 x i8> @llvm.genx.rdregioni.v4i8.v384i8.i16(<384 x i8> %[[ret_read1]], i32 0, i32 4, i32 1, i16 0, i32 undef)
  ; CHECK: %[[ret_elt1_copy:[^ ]+]] = call <4 x i8> @llvm.genx.wrregioni.v4i8.v4i8.i16.i1(<4 x i8> undef, <4 x i8> %[[ret_rdr1]], i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: %[[bitcast1:[^ ]+]] = bitcast <4 x i8> %[[ret_elt1_copy]] to float
  ; CHECK: %[[insert1:[^ ]+]] = insertvalue { float, <8 x float> } undef, float %[[bitcast1]], 0
  ; CHECK: %[[ret_read2:[^ ]+]] = call <384 x i8> @llvm.genx.read.predef.reg.v384i8.v384i8(i32 9, <384 x i8> undef)
  ; CHECK: %[[ret_rdr2:[^ ]+]] = call <32 x i8> @llvm.genx.rdregioni.v32i8.v384i8.i16(<384 x i8> %[[ret_read2]], i32 0, i32 32, i32 1, i16 4, i32 undef)
  ; CHECK: %[[ret_elt2_copy:[^ ]+]] = call <32 x i8> @llvm.genx.wrregioni.v32i8.v32i8.i16.i1(<32 x i8> undef, <32 x i8> %[[ret_rdr2]], i32 0, i32 32, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: %[[bitcast2:[^ ]+]] = bitcast <32 x i8> %[[ret_elt2_copy]] to <8 x float>
  ; CHECK: %[[insert2:[^ ]+]] = insertvalue { float, <8 x float> } %[[insert1]], <8 x float> %[[bitcast2]], 1
  ; CHECK: %[[extract1:[^ ]+]] = extractvalue { float, <8 x float> } %[[insert2]], 0
  ; CHECK: %[[extract2:[^ ]+]] = extractvalue { float, <8 x float> } %[[insert2]], 1
  %ret_extract0 = extractvalue {float, <8 x float>} %calltmp, 0
  %ret_extract1 = extractvalue {float, <8 x float>} %calltmp, 1
  %incval = call float @llvm.genx.rdregionf.f32.v8f32.i16(<8 x float> %ret_extract1, i32 0, i32 8, i32 1, i16 4, i32 undef)
  %newincval = fadd float %ret_extract0, %incval
  %result = call <8 x float> @llvm.genx.wrregionf.v8f32.f32.i16.i1(<8 x float> %ret_extract1, float %newincval, i32 0, i32 8, i32 1, i16 4, i32 undef, i1 true)
  %svm_st_ptrtoint = ptrtoint float* %RET to i64
  call void @llvm.genx.svm.block.st.i64.v8f32(i64 %svm_st_ptrtoint, <8 x float> %result)
  ret void
}

attributes #0 = { noinline nounwind readnone "CMStackCall" }
attributes #1 = { noinline nounwind "CMGenxMain" "oclrt"="1" }


!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}

!0 = !{i32 1, !"genx.useGlobalMem", i32 1}
!1 = !{i32 0, i32 0}
!genx.kernels = !{!2}
!genx.kernel.internal = !{!7}
!2 = !{void (float*, float*, i64)* @f_f, !"f_f", !3, i32 0, !4, !5, !6, i32 0}
!3 = !{i32 0, i32 0, i32 96}
!4 = !{i32 72, i32 80, i32 64}
!5 = !{i32 0, i32 0}
!6 = !{!"", !""}
!7 = !{void (float*, float*, i64)* @f_f, !8, !9, !10, null}
!8 = !{i32 0, i32 0, i32 0}
!9 = !{i32 0, i32 1, i32 2}
!10 = !{}
