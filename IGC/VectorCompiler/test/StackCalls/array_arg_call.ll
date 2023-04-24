;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrologEpilogInsertion -vc-arg-reg-size=32 -vc-ret-reg-size=12 \
; RUN: -mattr=+ocl_runtime -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <8 x float> @llvm.genx.svm.block.ld.v8f32.i64(i64)
declare void @llvm.genx.svm.block.st.i64.v8f32(i64, <8 x float>)

%array = type [2 x <4 x float>]

; CHECK-LABEL: sum_impl
define <4 x float> @sum_impl(%array %a) #0 {

; COM: read array from stack
; COM: elem 0
; CHECK: %[[CALLEE_PREDEF_READ_0:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[CALLEE_READ_ELEM_0_BYTES:[^ ]+]] = call <16 x i8> @llvm.genx.rdregioni.v16i8.v1024i8.i16(<1024 x i8> %[[CALLEE_PREDEF_READ_0]], i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: %[[CALLEE_ELEM_0:[^ ]+]] = bitcast <16 x i8> %[[CALLEE_READ_ELEM_0_BYTES]] to <4 x float>
; CHECK: %[[STR_INIT:[^ ]+]] = insertvalue [2 x <4 x float>] undef, <4 x float> %[[CALLEE_ELEM_0]], 0

; COM: elem 1
; CHECK: %[[CALLEE_PREDEF_READ_1:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[CALLEE_READ_ELEM_1_BYTES:[^ ]+]] = call <16 x i8> @llvm.genx.rdregioni.v16i8.v1024i8.i16(<1024 x i8> %[[CALLEE_PREDEF_READ_1]], i32 0, i32 16, i32 1, i16 16, i32 undef)
; CHECK: %[[CALLEE_ELEM_1:[^ ]+]] = bitcast <16 x i8> %[[CALLEE_READ_ELEM_1_BYTES]] to <4 x float>
; CHECK: %[[STR:[^ ]+]] = insertvalue [2 x <4 x float>] %[[STR_INIT]], <4 x float> %[[CALLEE_ELEM_1]], 1

; COM: users
; CHECK: extractvalue [2 x <4 x float>] %[[STR]], 0
; CHECK: extractvalue [2 x <4 x float>] %[[STR]], 1
  %first_half = extractvalue %array %a, 0
  %second_half = extractvalue %array %a, 1
  %res = fadd <4 x float> %first_half, %second_half

; COM: store ret on stack
; CHECK: %[[CALLEE_PREDEF_READ_2:[^ ]+]] = call <96 x float> @llvm.genx.read.predef.reg.v96f32.v96f32(i32 9, <96 x float> undef)
; CHECK: %[[CALLEE_WRITE_RES:[^ ]+]] = call <96 x float> @llvm.genx.wrregionf.v96f32.v4f32.i16.i1(<96 x float> %[[CALLEE_PREDEF_READ_2]], <4 x float> %res, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call <4 x float> @llvm.genx.write.predef.reg.v4f32.v96f32(i32 9, <96 x float> %[[CALLEE_WRITE_RES]])

  ret <4 x float> %res
}

; CHECK-LABEL: sum
define void @sum(float* %RET, float* %aFOO, i64 %privBase) #1 {

  %svm_ld_ptrtoint = ptrtoint float* %aFOO to i64
  %aFOO_block_ld = call <8 x float> @llvm.genx.svm.block.ld.v8f32.i64(i64 %svm_ld_ptrtoint)

  %first_half = shufflevector <8 x float> %aFOO_block_ld, <8 x float> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %second_half = shufflevector <8 x float> %aFOO_block_ld, <8 x float> undef, <4 x i32> <i32 4, i32 5, i32 6, i32 7>

  %array_arg_init = insertvalue %array undef, <4 x float> %first_half, 0
  %array_arg = insertvalue %array %array_arg_init, <4 x float> %second_half, 1

; COM: store array on stack
; COM: elem 0
; CHECK: %[[STR_ELEM_0:[^ ]+]] = extractvalue [2 x <4 x float>] %array_arg, 0
; CHECK: %[[STR_ELEM_0_BYTES:[^ ]+]] = bitcast <4 x float> %[[STR_ELEM_0]] to <16 x i8>
; CHECK: %[[PREDEF_READ_0:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[WRITE_ELEM_0:[^ ]+]] = call <1024 x i8> @llvm.genx.wrregioni.v1024i8.v16i8.i16.i1(<1024 x i8> %[[PREDEF_READ_0]], <16 x i8> %[[STR_ELEM_0_BYTES]], i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call <16 x i8> @llvm.genx.write.predef.reg.v16i8.v1024i8(i32 8, <1024 x i8> %[[WRITE_ELEM_0]])

; COM: elem 1
; CHECK: %[[STR_ELEM_1:[^ ]+]] = extractvalue [2 x <4 x float>] %array_arg, 1
; CHECK: %[[STR_ELEM_1_BYTES:[^ ]+]] = bitcast <4 x float> %[[STR_ELEM_1]] to <16 x i8>
; CHECK: %[[PREDEF_READ_1:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[WRITE_ELEM_1:[^ ]+]] = call <1024 x i8> @llvm.genx.wrregioni.v1024i8.v16i8.i16.i1(<1024 x i8> %[[PREDEF_READ_1]], <16 x i8> %[[STR_ELEM_1_BYTES]], i32 0, i32 16, i32 1, i16 16, i32 undef, i1 true)
; CHECK: call <16 x i8> @llvm.genx.write.predef.reg.v16i8.v1024i8(i32 8, <1024 x i8> %[[WRITE_ELEM_1]])

; COM: call
; CHECK: %[[CALL_RET:[^ ]+]] = call spir_func <4 x float> @sum_impl([2 x <4 x float>] %array_arg)
  %call_tmp = call spir_func <4 x float> @sum_impl(%array %array_arg)

; COM: take result from stack
; CHECK: %[[PREDEF_READ_2:[^ ]+]] = call <96 x float> @llvm.genx.read.predef.reg.v96f32.v96f32(i32 9, <96 x float> undef)
; CHECK: %[[READ_RET:[^ ]+]] = call <4 x float> @llvm.genx.rdregionf.v4f32.v96f32.i16(<96 x float> %[[PREDEF_READ_2]], i32 0, i32 4, i32 1, i16 0, i32 undef)
; CHECK: %[[RET_VAL:[^ ]+]] = call <4 x float> @llvm.genx.wrregionf.v4f32.v4f32.i16.i1(<4 x float> undef, <4 x float> %[[READ_RET]], i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)

; COM: output
; CHECK: shufflevector <4 x float> %[[RET_VAL]], <4 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3>
  %result = shufflevector <4 x float> %call_tmp, <4 x float> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 0, i32 1, i32 2, i32 3>

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
!2 = !{void (float*, float*, i64)* @sum, !"sum", !3, i32 0, !4, !5, !6, i32 0}
!3 = !{i32 0, i32 0, i32 96}
!4 = !{i32 72, i32 80, i32 64}
!5 = !{i32 0, i32 0}
!6 = !{!"", !""}
!7 = !{void (float*, float*, i64)* @sum, !8, !9, !10, null}
!8 = !{i32 0, i32 0, i32 0}
!9 = !{i32 0, i32 1, i32 2}
!10 = !{}
