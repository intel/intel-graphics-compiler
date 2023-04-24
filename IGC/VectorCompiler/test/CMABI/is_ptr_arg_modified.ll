;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>)
declare void @llvm.genx.svm.scatter.v1i1.v1i64.v1f64(<1 x i1>, i32, <1 x i64>, <1 x double>)
declare void @llvm.genx.svm.scatter.v4i1.v4i64.v4f64(<4 x i1>, i32, <4 x i64>, <4 x double>)
declare <1 x double> @llvm.genx.svm.gather.v1f64.v1i1.v1i64(<1 x i1>, i32, <1 x i64>, <1 x double>)
declare <4 x double> @llvm.genx.svm.gather.v4f64.v4i1.v4i64(<4 x i1>, i32, <4 x i64>, <4 x double>)

; Function Attrs: noinline nounwind
define internal spir_func void @use_store(<8 x i32>* %vector.ref) {
  %ptr = getelementptr inbounds <8 x i32>, <8 x i32>* %vector.ref, i64 0
  %pseudo.use = ptrtoint <8 x i32>* %ptr to i64
  store <8 x i32> zeroinitializer, <8 x i32>* %vector.ref
  %ptr.2 = getelementptr inbounds <8 x i32>, <8 x i32>* %vector.ref, i64 0
  %pseudo.use.2 = ptrtoint <8 x i32>* %ptr.2 to i64
  ret void

; COM: to make use list: {%ptr, store, %ptr.2}
  uselistorder <8 x i32>* %vector.ref, {2, 1, 0}
}

define internal spir_func void @use_svm_scatter(<1 x double>* %vector.ptr) {
  %vector.ptr.int = ptrtoint <1 x double>* %vector.ptr to i64
  %addr.vec = insertelement <1 x i64> undef, i64 %vector.ptr.int, i32 0
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1f64(<1 x i1> <i1 true>, i32 0, <1 x i64> %addr.vec, <1 x double> zeroinitializer)
  ret void
}

define internal spir_func void @use_svm_scatter2(<4 x double>* %vector.ptr) {
  %vector.ptr.int = ptrtoint <4 x double>* %vector.ptr to i64
  %addr.vec1 = insertelement <1 x i64> undef, i64 %vector.ptr.int, i32 0
  %addr.vec2 = shufflevector <1 x i64> %addr.vec1, <1 x i64> undef, <4 x i32> zeroinitializer
  %addr.vec3 = add <4 x i64> %addr.vec2, <i64 0, i64 8, i64 16, i64 24>
  call void @llvm.genx.svm.scatter.v4i1.v4i64.v4f64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 0, <4 x i64> %addr.vec3, <4 x double> zeroinitializer)
  ret void
}

define internal spir_func void @use_svm_gather(<1 x double>* %vector.ptr) {
  %vector.ptr.int = ptrtoint <1 x double>* %vector.ptr to i64
  %addr.vec = insertelement <1 x i64> undef, i64 %vector.ptr.int, i32 0
  %res = call <1 x double> @llvm.genx.svm.gather.v1f64.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %addr.vec, <1 x double> undef)
  ret void
}

define internal spir_func void @use_svm_gather2(<4 x double>* %vector.ptr) {
  %vector.ptr.int = ptrtoint <4 x double>* %vector.ptr to i64
  %addr.vec1 = insertelement <1 x i64> undef, i64 %vector.ptr.int, i32 0
  %addr.vec2 = shufflevector <1 x i64> %addr.vec1, <1 x i64> undef, <4 x i32> zeroinitializer
  %addr.vec3 = add <4 x i64> %addr.vec2, <i64 0, i64 8, i64 16, i64 24>
  %res = call <4 x double> @llvm.genx.svm.gather.v4f64.v4i1.v4i64(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 0, <4 x i64> %addr.vec3, <4 x double> undef)
  ret void
}

define internal spir_func void @use_store_after_bitcast(<16 x double>* %vector.ptr) {
  %elem.ptr = getelementptr inbounds <16 x double>, <16 x double>* %vector.ptr, i64 0, i64 0
  %elem.ptr.cast = bitcast double* %elem.ptr to i64*
  store i64 0, i64* %elem.ptr.cast
  ret void
}

define internal spir_func void @use_load_after_bitcast(<16 x double>* %vector.ptr) {
  %elem.ptr = getelementptr inbounds <16 x double>, <16 x double>* %vector.ptr, i64 0, i64 0
  %elem.ptr.cast = bitcast double* %elem.ptr to i64*
  %res = load i64, i64* %elem.ptr.cast
  ret void
}

define internal spir_func void @use_gather_and_scatter(<1 x double>* %vector.ptr) {
  %vector.ptr.int = ptrtoint <1 x double>* %vector.ptr to i64
  %addr.vec = insertelement <1 x i64> undef, i64 %vector.ptr.int, i32 0
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1f64(<1 x i1> <i1 true>, i32 0, <1 x i64> %addr.vec, <1 x double> zeroinitializer)
  %res = call <1 x double> @llvm.genx.svm.gather.v1f64.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %addr.vec, <1 x double> undef)
  ret void
}

; Function Attrs: noinline nounwind
define dllexport void @kernel() {
  %vec1.mem = alloca <8 x i32>, align 32
  call spir_func void @use_store(<8 x i32>* nonnull %vec1.mem)
; CHECK: %[[RET1:[^ ]+]] = call spir_func <8 x i32> @use_store(<8 x i32> %vec1.mem.val)
; CHECK: store <8 x i32> %[[RET1]], <8 x i32>* %vec1.mem
  %vec1 = load <8 x i32>, <8 x i32>* %vec1.mem, align 32
  call void @llvm.genx.oword.st.v8i32(i32 0, i32 0, <8 x i32> %vec1)

  %vec2.mem = alloca <1 x double>, align 32
  call spir_func void @use_svm_scatter(<1 x double>* %vec2.mem)
; CHECK: %[[RET2:[^ ]+]] = call spir_func <1 x double> @use_svm_scatter(<1 x double> %vec2.mem.val)
; CHECK: store <1 x double> %[[RET2]], <1 x double>* %vec2.mem

  %vec3.mem = alloca <4 x double>, align 32
  call spir_func void @use_svm_scatter2(<4 x double>* %vec3.mem)
; CHECK: %[[RET3:[^ ]+]] = call spir_func <4 x double> @use_svm_scatter2(<4 x double> %vec3.mem.val)
; CHECK: store <4 x double> %[[RET3]], <4 x double>* %vec3.mem

  %vec4.mem = alloca <1 x double>, align 32
  call spir_func void @use_svm_gather(<1 x double>* %vec4.mem)
; CHECK: call spir_func void @use_svm_gather(<1 x double> %vec4.mem.val)

  %vec5.mem = alloca <4 x double>, align 32
  call spir_func void @use_svm_gather2(<4 x double>* %vec5.mem)
; CHECK: call spir_func void @use_svm_gather2(<4 x double> %vec5.mem.val)

  %vec6.mem = alloca <16 x double>
  call spir_func void @use_store_after_bitcast(<16 x double>* %vec6.mem)
; CHECK: %[[RET6:[^ ]+]] = call spir_func <16 x double> @use_store_after_bitcast(<16 x double> %vec6.mem.val)
; CHECK: store <16 x double> %[[RET6]], <16 x double>* %vec6.mem

  %vec7.mem = alloca <16 x double>
  call spir_func void @use_load_after_bitcast(<16 x double>* %vec7.mem)
; CHECK: call spir_func void @use_load_after_bitcast(<16 x double> %vec7.mem.val)

  %vec8.mem = alloca <1 x double>, align 32
  call spir_func void @use_gather_and_scatter(<1 x double>* %vec8.mem)
; CHECK: %[[RET8:[^ ]+]] = call spir_func <1 x double> @use_gather_and_scatter(<1 x double> %vec8.mem.val)
; CHECK: store <1 x double> %[[RET8]], <1 x double>* %vec8.mem

  ret void
}

!genx.kernels = !{!0}
!0 = !{void ()* @kernel}
