;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

; ModuleID = 'memset_kernels'
source_filename = "memset_kernels.ll"

declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg)
declare void @llvm.memset.p1i8.i64(i8 addrspace(1)* nocapture writeonly, i8, i64, i1 immarg)

%"class::buf" = type { [8 x i16] }

define void @kernel_gep(%"class::buf" addrspace(1)* %0) {
entry:
; CHECK-LABEL: define void @kernel_gep
; CHECK: [[BCAST:%.*]] = bitcast %"class::buf" addrspace(1)* [[GEP:%.*]] to <16 x i16> addrspace(1)*
; CHECK: [[GEP0:%.*]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[BCAST]], i32 0
; CHECK: store <16 x i16> zeroinitializer, <16 x i16> addrspace(1)* [[GEP0]], align 16
; CHECK: [[GEP1:%.*]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[BCAST]], i32 1
; CHECK: store <16 x i16> zeroinitializer, <16 x i16> addrspace(1)* [[GEP1]], align 16
; CHECK: [[GEP2:%.*]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[BCAST]], i32 2
; CHECK: store <16 x i16> zeroinitializer, <16 x i16> addrspace(1)* [[GEP2]], align 16
; CHECK: [[GEP3:%.*]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[BCAST]], i32 3
; CHECK: store <16 x i16> zeroinitializer, <16 x i16> addrspace(1)* [[GEP3]], align 16
; CHECK: [[GEP4:%.*]] = getelementptr <16 x i16>, <16 x i16> addrspace(1)* [[BCAST]], i32 4
; CHECK: store <16 x i16> zeroinitializer, <16 x i16> addrspace(1)* [[GEP4]], align 16
  %gep = getelementptr inbounds %"class::buf", %"class::buf" addrspace(1)* %0, i64 0
  %bcast = bitcast %"class::buf" addrspace(1)* %gep to i8 addrspace(1)*
  call void @llvm.memset.p1i8.i64(i8 addrspace(1)* align 16 %bcast, i8 0, i64 160, i1 false)
  ret void
}

define void @kernel_alloca() {
entry:
; CHECK-LABEL: define void @kernel_alloca
; CHECK: [[BC:%.*]] = bitcast [32 x i32]* %buf to <8 x i32>*
; CHECK: [[GEP0:%.*]] = getelementptr <8 x i32>, <8 x i32>* [[BC]], i32 0
; CHECK: store <8 x i32> zeroinitializer, <8 x i32>* [[GEP0]], align 4
; CHECK: [[GEP1:%.*]] = getelementptr <8 x i32>, <8 x i32>* [[BC]], i32 1
; CHECK: store <8 x i32> zeroinitializer, <8 x i32>* [[GEP1]], align 4
; CHECK: [[GEP2:%.*]] = getelementptr <8 x i32>, <8 x i32>* [[BC]], i32 2
; CHECK: store <8 x i32> zeroinitializer, <8 x i32>* [[GEP2]], align 4
  %buf = alloca [32 x i32], align 2
  %ptr = bitcast [32 x i32]* %buf to i8*
  call void @llvm.memset.p0i8.i64(i8* align 4 %ptr, i8 0, i64 128, i1 false)
  ret void
}

define void @kernel_alloca_2() {
entry:
; CHECK-LABEL: define void @kernel_alloca_2
; CHECK: [[BC:%.*]] = bitcast [32 x i16]* %buf to <16 x i16>*
; CHECK: [[GEP0:%.*]] = getelementptr <16 x i16>, <16 x i16>* [[BC]], i32 0
; CHECK: store <16 x i16> zeroinitializer, <16 x i16>* [[GEP0]], align 4
; CHECK: [[GEP1:%.*]] = getelementptr <16 x i16>, <16 x i16>* [[BC]], i32 1
; CHECK: store <16 x i16> zeroinitializer, <16 x i16>* [[GEP1]], align 4
  %buf = alloca [32 x i16], align 2
  %ptr = bitcast [32 x i16]* %buf to i8*
  call void @llvm.memset.p0i8.i64(i8* align 4 %ptr, i8 0, i64 64, i1 false)
  ret void
}
