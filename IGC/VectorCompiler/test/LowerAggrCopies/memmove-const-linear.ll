;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -genx-lower-aggr-copies -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefix=CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -genx-lower-aggr-copies -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefix=CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare void @llvm.memmove.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1)

define internal spir_func void @foo(i8* %dst, i8* %src) {
  ; CHECK-TYPED-PTRS: [[src1:%[^ ]+]] = bitcast i8* %src to <1 x i8>*
  ; CHECK-TYPED-PTRS: [[data1:%[^ ]+]] = load <1 x i8>, <1 x i8>* [[src1]]
  ; CHECK-TYPED-PTRS: [[dst1:%[^ ]+]] = bitcast i8* %dst to <1 x i8>*
  ; CHECK-TYPED-PTRS: store <1 x i8> [[data1]], <1 x i8>* [[dst1]]
  ; CHECK-OPAQUE-PTRS: [[data1:%[^ ]+]] = load <1 x i8>, ptr %src
  ; CHECK-OPAQUE-PTRS: store <1 x i8> [[data1]], ptr %dst
  call void @llvm.memmove.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 1, i1 false)

  ; CHECK-TYPED-PTRS: [[src2:%[^ ]+]] = bitcast i8* %src to <8 x i8>*
  ; CHECK-TYPED-PTRS: [[data2:%[^ ]+]] = load <8 x i8>, <8 x i8>* [[src2]], align 8
  ; CHECK-TYPED-PTRS: [[dst2:%[^ ]+]] = bitcast i8* %dst to <8 x i8>*
  ; CHECK-TYPED-PTRS: store <8 x i8> [[data2]], <8 x i8>* [[dst2]]
  ; CHECK-OPAQUE-PTRS: [[data2:%[^ ]+]] = load <8 x i8>, ptr %src, align 8
  ; CHECK-OPAQUE-PTRS: store <8 x i8> [[data2]], ptr %dst
  call void @llvm.memmove.p0i8.p0i8.i64(i8* %dst, i8* align 8 %src, i64 8, i1 false)

  ; CHECK-TYPED-PTRS: [[src3:%[^ ]+]] = bitcast i8* %src to <8 x i8>*
  ; CHECK-TYPED-PTRS: [[data3:%[^ ]+]] = load <8 x i8>, <8 x i8>* [[src3]]
  ; CHECK-TYPED-PTRS: [[dst3:%[^ ]+]] = bitcast i8* %dst to <8 x i8>*
  ; CHECK-TYPED-PTRS: store <8 x i8> [[data3]], <8 x i8>* [[dst3]], align 8
  ; CHECK-OPAQUE-PTRS: [[data3:%[^ ]+]] = load <8 x i8>, ptr %src
  ; CHECK-OPAQUE-PTRS: store <8 x i8> [[data3]], ptr %dst, align 8
  call void @llvm.memmove.p0i8.p0i8.i64(i8* align 8 %dst, i8* %src, i64 8, i1 false)

  ; CHECK-TYPED-PTRS: [[src4:%[^ ]+]] = bitcast i8* %src to <8 x i8>*
  ; CHECK-TYPED-PTRS: [[data4:%[^ ]+]] = load <8 x i8>, <8 x i8>* [[src4]], align 8
  ; CHECK-TYPED-PTRS: [[dst4:%[^ ]+]] = bitcast i8* %dst to <8 x i8>*
  ; CHECK-TYPED-PTRS: store <8 x i8> [[data4]], <8 x i8>* [[dst4]], align 8
  ; CHECK-OPAQUE-PTRS: [[data4:%[^ ]+]] = load <8 x i8>, ptr %src, align 8
  ; CHECK-OPAQUE-PTRS: store <8 x i8> [[data4]], ptr %dst, align 8
  call void @llvm.memmove.p0i8.p0i8.i64(i8* align 8 %dst, i8* align 8 %src, i64 8, i1 false)

  ret void
}
