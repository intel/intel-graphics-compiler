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
declare i32 @llvm.genx.get.hwid()

%"class.A" = type { <8 x float> }

; COM: basic alloca replacement rule
; COM: alloca -> addr = Sp + padding; ptr = inttoptr(addr); Sp = addr + alloca_size;
define <8 x float>* @replace_alloca(i8* %p) {
; CHECK-LABEL: replace_alloca
; COM: Let's also check the order of main stack-related operations.
; COM: More interesting allocation tests are in a separate file.

; COM: for subroutines we save SP in the prolog, to restore it before function returns.
; CHECK: %[[SPREAD:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: %[[SP:[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %[[SPREAD]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[SPCOPY:[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> %[[SP]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[SPSCALAR:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[SPCOPY]], i32 0, i32 1, i32 1, i16 0, i32 undef)

; COM: run-time alloca alignment
; COM: SP = SP + (required alignment - 1)
; COM: SP = SP & ~(required alignment - 1)
; COM: sequence for addition
; CHECK: %[[SP2READ:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: %[[SP2:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[SP2READ]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[ADD:[^ ]+]] = add i64 %[[SP2]], 31
; CHECK: %[[ADDWRR:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[ADD]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %[[ADDWRR]])
; COM: sequence for bitwise and
; CHECK: %[[SP3READ:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: %[[SP3:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[SP3READ]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[AND:[^ ]+]] = and i64 %[[SP3]], -32
; CHECK: %[[ANDWRR:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[AND]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %[[ANDWRR]])

; COM: Now check memory allocation
; CHECK: %[[SP4READ:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
; CHECK: %[[SP4:[^ ]+]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v1i64.i16(<1 x i64> %[[SP4READ]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[SP4COPY:[^ ]+]] = call <1 x i64> @llvm.genx.wrregioni.v1i64.v1i64.i16.i1(<1 x i64> undef, <1 x i64> %[[SP4]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[SP4SCALAR:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[SP4COPY]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK: %[[ALLOCATE:[^ ]+]] = add i64 %[[SP4SCALAR]], 32
; CHECK: %[[SP4WRR:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[ALLOCATE]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %[[SP4WRR]])
; CHECK: %[[USE:[^ ]+]] = inttoptr i64 %[[SP4SCALAR]] to %class.A*
; CHECK: getelementptr inbounds %class.A, %class.A* %[[USE]], i64 0, i32 0

  %alc = alloca %"class.A", align 32
  %elt = getelementptr inbounds %"class.A", %"class.A"* %alc, i64 0, i32 0

; COM: restore SP
; CHECK: %[[SPWRR:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[SPSCALAR]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[SPWRITE:[^ ]+]] = call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %[[SPWRR]])
  ret <8 x float>* %elt
}


define <8 x float> @foo___vyfvyf(<8 x float> %a, <8 x float> %b) #0 {
  ; CHECK-LABEL: foo___vyfvyf
  ; CHECK: %[[arg0_f:[^ ]+]] = call <256 x float> @llvm.genx.read.predef.reg.v256f32.v256f32(i32 8, <256 x float> undef)
  ; CHECK: %[[arg0rdr_f:[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v256f32.i16(<256 x float> %[[arg0_f]], i32 0, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK: %[[arg1_f:[^ ]+]] = call <256 x float> @llvm.genx.read.predef.reg.v256f32.v256f32(i32 8, <256 x float> undef)
  ; CHECK: %[[arg1rdr_f:[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v256f32.i16(<256 x float> %[[arg1_f]], i32 0, i32 8, i32 1, i16 32, i32 undef)
  %add_a_load_b_load = fadd <8 x float> %a, %b
  ; CHECK: %[[retval_f:[^ ]+]] = fadd <8 x float> %[[arg0rdr_f]], %[[arg1rdr_f]]
  ; CHECK: %[[retvalreg_f:[^ ]+]] = call <96 x float> @llvm.genx.read.predef.reg.v96f32.v96f32(i32 9, <96 x float> undef
  ; CHECK: %[[retvalwrr_f:[^ ]+]] = call <96 x float> @llvm.genx.wrregionf.v96f32.v8f32.i16.i1(<96 x float> %[[retvalreg_f]], <8 x float> %[[retval_f]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: %[[retvalpredef_f:[^ ]+]] = call <8 x float> @llvm.genx.write.predef.reg.v8f32.v96f32(i32 9, <96 x float> %[[retvalwrr_f]])
  ; CHECK: ret <8 x float> %[[retval_f]]
  ret <8 x float> %add_a_load_b_load
}

define void @f_f(float* %RET, float* %aFOO, i64 %privBase) #1 {
  ; CHECK-LABEL: f_f
  ; CHECK: %[[hwid:[^ ]+]] = call i32 @llvm.vc.internal.logical.thread.id()
  ; CHECK: %[[hwid_mul:[^ ]+]] = mul i32 %[[hwid]], 8192
  ; CHECK: %[[hwid_ext:[^ ]+]] = zext i32 %[[hwid_mul]] to i64
  ; CHECK: %[[sp:[^ ]+]] = add i64 %privBase, %[[hwid_ext]]
  ; COM: initialize SP
  ; CHECK: %[[spwrr:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[sp]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: call i64 @llvm.genx.write.predef.reg.i64.i64(i32 10, i64 %[[spwrr]])
  ; COM: initialize FP = SP
  ; CHECK: %[[sp:[^ ]+]] = call <1 x i64> @llvm.genx.read.predef.reg.v1i64.i64(i32 10, i64 undef)
  ; CHECK: %[[sprdr:[^ ]+]] = call i64 @llvm.genx.rdregioni.i64.v1i64.i16(<1 x i64> %[[sp]], i32 0, i32 1, i32 1, i16 0, i32 undef)
  ; CHECK: %[[fpwrr:[^ ]+]] = call i64 @llvm.genx.wrregioni.i64.i64.i16.i1(i64 undef, i64 %[[sprdr]], i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: = call i64 @llvm.genx.write.predef.reg.i64.i64(i32 11, i64 %[[fpwrr]])
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
  ; CHECK: %[[arg0write:[^ ]+]] = call <8 x float> @llvm.genx.write.predef.reg.v8f32.v256f32(i32 8, <256 x float> %[[arg0wrr]])
  ; CHECK: %[[arg1:[^ ]+]] = call <256 x float> @llvm.genx.read.predef.reg.v256f32.v256f32(i32 8, <256 x float> undef)
  ; CHECK: %[[arg1wrr:[^ ]+]] = call <256 x float> @llvm.genx.wrregionf.v256f32.v8f32.i16.i1(<256 x float> %[[arg1]], <8 x float> %sub_aFOO_load8_offset_load__broadcast11, i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK: %[[arg1write:[^ ]+]] = call <8 x float> @llvm.genx.write.predef.reg.v8f32.v256f32(i32 8, <256 x float> %[[arg1wrr]])
  %calltmp = call spir_func <8 x float> @foo___vyfvyf(<8 x float> %aFOO_load_ptr2int_2void2122_masked_load23, <8 x float> %sub_aFOO_load8_offset_load__broadcast11)
  ; CHECK: %[[call:[^ ]+]] = call spir_func <8 x float> @foo___vyfvyf(<8 x float> %aFOO_load_ptr2int_2void2122_masked_load23, <8 x float> %sub_aFOO_load8_offset_load__broadcast11)
  %svm_st_ptrtoint = ptrtoint float* %RET to i64
  call void @llvm.genx.svm.block.st.i64.v8f32(i64 %svm_st_ptrtoint, <8 x float> %calltmp)
  ; CHECK: %[[retval:[^ ]+]] = call <96 x float> @llvm.genx.read.predef.reg.v96f32.v96f32(i32 9, <96 x float> undef)
  ; CHECK: %[[retvalrdr:[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v96f32.i16(<96 x float> %[[retval]], i32 0, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK: %[[retvalcopy:[^ ]+]] = call <8 x float> @llvm.genx.wrregionf.v8f32.v8f32.i16.i1(<8 x float> undef, <8 x float> %[[retvalrdr]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: call void @llvm.genx.svm.block.st.i64.v8f32(i64 %svm_st_ptrtoint, <8 x float> %[[retvalcopy]])
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
