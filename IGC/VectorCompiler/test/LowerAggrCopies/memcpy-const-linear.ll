;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -genx-lower-aggr-copies -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1)

define internal spir_func void @foo(i8* %dst, i8* %src) {
  ; CHECK: [[src1:%[^ ]+]] = bitcast i8* %src to <1 x i8>*
  ; CHECK: [[data1:%[^ ]+]] = load <1 x i8>, <1 x i8>* [[src1]]
  ; CHECK: [[dst1:%[^ ]+]] = bitcast i8* %dst to <1 x i8>*
  ; CHECK: store <1 x i8> [[data1]], <1 x i8>* [[dst1]]
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 1, i1 false)

  ; CHECK: [[src2:%[^ ]+]] = bitcast i8* %src to <8 x i8>*
  ; CHECK: [[data2:%[^ ]+]] = load <8 x i8>, <8 x i8>* [[src2]], align 8
  ; CHECK: [[dst2:%[^ ]+]] = bitcast i8* %dst to <8 x i8>*
  ; CHECK: store <8 x i8> [[data2]], <8 x i8>* [[dst2]]
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* align 8 %src, i64 8, i1 false)

  ; CHECK: [[src3:%[^ ]+]] = bitcast i8* %src to <8 x i8>*
  ; CHECK: [[data3:%[^ ]+]] = load <8 x i8>, <8 x i8>* [[src3]]
  ; CHECK: [[dst3:%[^ ]+]] = bitcast i8* %dst to <8 x i8>*
  ; CHECK: store <8 x i8> [[data3]], <8 x i8>* [[dst3]], align 8
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %dst, i8* %src, i64 8, i1 false)

  ; CHECK: [[src4:%[^ ]+]] = bitcast i8* %src to <8 x i8>*
  ; CHECK: [[data4:%[^ ]+]] = load <8 x i8>, <8 x i8>* [[src4]], align 8
  ; CHECK: [[dst4:%[^ ]+]] = bitcast i8* %dst to <8 x i8>*
  ; CHECK: store <8 x i8> [[data4]], <8 x i8>* [[dst4]], align 8
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %dst, i8* align 8 %src, i64 8, i1 false)

  ret void
}
