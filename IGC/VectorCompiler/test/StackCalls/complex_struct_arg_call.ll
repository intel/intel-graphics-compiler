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

%struct_element = type [2 x { <2 x float> }]
%struct = type { %struct_element, %struct_element }

; CHECK-LABEL: sum_impl
define <4 x float> @sum_impl(%struct %s) #0 {

; COM: read struct from stack
; COM: elem 0 0
; CHECK: %[[CALLEE_PREDEF_READ_00:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[CALLEE_READ_ELEM_00_BYTES:[^ ]+]] = call <8 x i8> @llvm.genx.rdregioni.v8i8.v1024i8.i16(<1024 x i8> %[[CALLEE_PREDEF_READ_00]], i32 0, i32 8, i32 1, i16 0, i32 undef)
; CHECK: %[[CALLEE_ELEM_00:[^ ]+]] = bitcast <8 x i8> %[[CALLEE_READ_ELEM_00_BYTES]] to <2 x float>
; CHECK: %[[STR_0:[^ ]+]] = insertvalue %struct undef, <2 x float> %[[CALLEE_ELEM_00]], 0, 0, 0

; COM: elem 0 1
; CHECK: %[[CALLEE_PREDEF_READ_01:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[CALLEE_READ_ELEM_01_BYTES:[^ ]+]] = call <8 x i8> @llvm.genx.rdregioni.v8i8.v1024i8.i16(<1024 x i8> %[[CALLEE_PREDEF_READ_01]], i32 0, i32 8, i32 1, i16 8, i32 undef)
; CHECK: %[[CALLEE_ELEM_01:[^ ]+]] = bitcast <8 x i8> %[[CALLEE_READ_ELEM_01_BYTES]] to <2 x float>
; CHECK: %[[STR_1:[^ ]+]] = insertvalue %struct %[[STR_0]], <2 x float> %[[CALLEE_ELEM_01]], 0, 1, 0

; COM: elem 1 0
; CHECK: %[[CALLEE_PREDEF_READ_10:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[CALLEE_READ_ELEM_10_BYTES:[^ ]+]] = call <8 x i8> @llvm.genx.rdregioni.v8i8.v1024i8.i16(<1024 x i8> %[[CALLEE_PREDEF_READ_10]], i32 0, i32 8, i32 1, i16 16, i32 undef)
; CHECK: %[[CALLEE_ELEM_10:[^ ]+]] = bitcast <8 x i8> %[[CALLEE_READ_ELEM_10_BYTES]] to <2 x float>
; CHECK: %[[STR_2:[^ ]+]] = insertvalue %struct %[[STR_1]], <2 x float> %[[CALLEE_ELEM_10]], 1, 0, 0

; COM: elem 1 1
; CHECK: %[[CALLEE_PREDEF_READ_11:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[CALLEE_READ_ELEM_11_BYTES:[^ ]+]] = call <8 x i8> @llvm.genx.rdregioni.v8i8.v1024i8.i16(<1024 x i8> %[[CALLEE_PREDEF_READ_11]], i32 0, i32 8, i32 1, i16 24, i32 undef)
; CHECK: %[[CALLEE_ELEM_11:[^ ]+]] = bitcast <8 x i8> %[[CALLEE_READ_ELEM_11_BYTES]] to <2 x float>
; CHECK: %[[STR:[^ ]+]] = insertvalue %struct %[[STR_2]], <2 x float> %[[CALLEE_ELEM_11]], 1, 1, 0

; COM: users
; CHECK: extractvalue %struct %[[STR]], 0
; CHECK: extractvalue %struct %[[STR]], 1
  %first_half = extractvalue %struct %s, 0
  %second_half = extractvalue %struct %s, 1

; COM: store ret on stack
; CHECK: %[[CALLEE_PREDEF_READ_2:[^ ]+]] = call <96 x float> @llvm.genx.read.predef.reg.v96f32.v96f32(i32 9, <96 x float> undef)
; CHECK: %[[CALLEE_WRITE_RES:[^ ]+]] = call <96 x float> @llvm.genx.wrregionf.v96f32.v4f32.i16.i1(<96 x float> %[[CALLEE_PREDEF_READ_2]], <4 x float> undef, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call <4 x float> @llvm.genx.write.predef.reg.v4f32.v96f32(i32 9, <96 x float> %[[CALLEE_WRITE_RES]])

  ret <4 x float> undef
}

; CHECK-LABEL: sum
define void @sum(float* %RET, float* %aFOO, i64 %privBase) #1 {

  %svm_ld_ptrtoint = ptrtoint float* %aFOO to i64
  %aFOO_block_ld = call <8 x float> @llvm.genx.svm.block.ld.v8f32.i64(i64 %svm_ld_ptrtoint)

  %struct_0 = insertvalue %struct undef, <2 x float> <float 1.0, float 1.5>, 0, 0, 0
  %struct_1 = insertvalue %struct %struct_0, <2 x float> <float 2.0, float 2.5>, 0, 1, 0
  %struct_2 = insertvalue %struct %struct_1, <2 x float> <float 3.0, float 3.5>, 1, 0, 0
  %struct_arg = insertvalue %struct %struct_2, <2 x float> <float 4.0, float 4.5>, 1, 1, 0

; COM: store struct on stack
; COM: elem 0 0
; CHECK: %[[STR_ELEM_00:[^ ]+]] = extractvalue %struct %struct_arg, 0, 0, 0
; CHECK: %[[STR_ELEM_00_BYTES:[^ ]+]] = bitcast <2 x float> %[[STR_ELEM_00]] to <8 x i8>
; CHECK: %[[PREDEF_READ_00:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[WRITE_ELEM_00:[^ ]+]] = call <1024 x i8> @llvm.genx.wrregioni.v1024i8.v8i8.i16.i1(<1024 x i8> %[[PREDEF_READ_00]], <8 x i8> %[[STR_ELEM_00_BYTES]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK: call <8 x i8> @llvm.genx.write.predef.reg.v8i8.v1024i8(i32 8, <1024 x i8> %[[WRITE_ELEM_00]])

; COM: elem 0 1
; CHECK: %[[STR_ELEM_01:[^ ]+]] = extractvalue %struct %struct_arg, 0, 1, 0
; CHECK: %[[STR_ELEM_01_BYTES:[^ ]+]] = bitcast <2 x float> %[[STR_ELEM_01]] to <8 x i8>
; CHECK: %[[PREDEF_READ_01:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[WRITE_ELEM_01:[^ ]+]] = call <1024 x i8> @llvm.genx.wrregioni.v1024i8.v8i8.i16.i1(<1024 x i8> %[[PREDEF_READ_01]], <8 x i8> %[[STR_ELEM_01_BYTES]], i32 0, i32 8, i32 1, i16 8, i32 undef, i1 true)
; CHECK: call <8 x i8> @llvm.genx.write.predef.reg.v8i8.v1024i8(i32 8, <1024 x i8> %[[WRITE_ELEM_01]])

; COM: elem 1 0
; CHECK: %[[STR_ELEM_10:[^ ]+]] = extractvalue %struct %struct_arg, 1, 0, 0
; CHECK: %[[STR_ELEM_10_BYTES:[^ ]+]] = bitcast <2 x float> %[[STR_ELEM_10]] to <8 x i8>
; CHECK: %[[PREDEF_READ_10:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[WRITE_ELEM_10:[^ ]+]] = call <1024 x i8> @llvm.genx.wrregioni.v1024i8.v8i8.i16.i1(<1024 x i8> %[[PREDEF_READ_10]], <8 x i8> %[[STR_ELEM_10_BYTES]], i32 0, i32 8, i32 1, i16 16, i32 undef, i1 true)
; CHECK: call <8 x i8> @llvm.genx.write.predef.reg.v8i8.v1024i8(i32 8, <1024 x i8> %[[WRITE_ELEM_10]])

; COM: elem 1 1
; CHECK: %[[STR_ELEM_11:[^ ]+]] = extractvalue %struct %struct_arg, 1, 1, 0
; CHECK: %[[STR_ELEM_11_BYTES:[^ ]+]] = bitcast <2 x float> %[[STR_ELEM_11]] to <8 x i8>
; CHECK: %[[PREDEF_READ_11:[^ ]+]] = call <1024 x i8> @llvm.genx.read.predef.reg.v1024i8.v1024i8(i32 8, <1024 x i8> undef)
; CHECK: %[[WRITE_ELEM_11:[^ ]+]] = call <1024 x i8> @llvm.genx.wrregioni.v1024i8.v8i8.i16.i1(<1024 x i8> %[[PREDEF_READ_11]], <8 x i8> %[[STR_ELEM_11_BYTES]], i32 0, i32 8, i32 1, i16 24, i32 undef, i1 true)
; CHECK: call <8 x i8> @llvm.genx.write.predef.reg.v8i8.v1024i8(i32 8, <1024 x i8> %[[WRITE_ELEM_11]])

; COM: call
; CHECK: %[[CALL_RET:[^ ]+]] = call spir_func <4 x float> @sum_impl(%struct %struct_arg)
  %call_tmp = call spir_func <4 x float> @sum_impl(%struct %struct_arg)

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
