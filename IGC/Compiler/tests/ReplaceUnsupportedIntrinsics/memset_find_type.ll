;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus

; RUN: igc_opt --opaque-pointers -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

; For opaque pointers we need to be able to deduce the type
; and we can find it by investigating instructions

; ModuleID = 'memset_kernels'
source_filename = "memset_kernels.ll"

declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg)
declare void @llvm.memset.p1.i64(ptr addrspace(1) nocapture writeonly, i8, i64, i1 immarg)
declare void @llvm.memset.p3.i64(ptr addrspace(3) nocapture writeonly, i8, i64, i1 immarg)

%"class::buf" = type { [8 x i16] }

@global_val = external addrspace(3) global [0 x i8]

define void @kernel_global() {
entry:
; CHECK-LABEL: define void @kernel_global
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) [[GV:@.*]], align 32
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) getelementptr inbounds (<8 x i32>, ptr addrspace(3) [[GV]], i32 1), align 32
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) getelementptr (<8 x i32>, ptr addrspace(3) [[GV]], i32 2), align 32
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) getelementptr (<8 x i32>, ptr addrspace(3) [[GV]], i32 3), align 32
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) getelementptr (<8 x i32>, ptr addrspace(3) [[GV]], i32 4), align 32
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) getelementptr (<8 x i32>, ptr addrspace(3) [[GV]], i32 5), align 32
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) getelementptr (<8 x i32>, ptr addrspace(3) [[GV]], i32 6), align 32
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) getelementptr (<8 x i32>, ptr addrspace(3) [[GV]], i32 7), align 32
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) getelementptr (<8 x i32>, ptr addrspace(3) [[GV]], i32 8), align 32
; CHECK: store <8 x i32> zeroinitializer, ptr addrspace(3) getelementptr (<8 x i32>, ptr addrspace(3) [[GV]], i32 9), align 32
  call void @llvm.memset.p3.i64(ptr addrspace(3) align 32 @global_val, i8 0, i64 320, i1 false)
  ret void;
}

define void @kernel_gep(ptr addrspace(1) %0) {
entry:
; CHECK-LABEL: define void @kernel_gep
; CHECK: [[GEP0:%.*]] = getelementptr <16 x i16>, ptr addrspace(1) [[GEP:%.*]], i32 0
; CHECK: store <16 x i16> zeroinitializer, ptr addrspace(1) [[GEP0]], align 16
; CHECK: [[GEP1:%.*]] = getelementptr <16 x i16>, ptr addrspace(1) [[GEP]], i32 1
; CHECK: store <16 x i16> zeroinitializer, ptr addrspace(1) [[GEP1]], align 16
; CHECK: [[GEP2:%.*]] = getelementptr <16 x i16>, ptr addrspace(1) [[GEP]], i32 2
; CHECK: store <16 x i16> zeroinitializer, ptr addrspace(1) [[GEP2]], align 16
; CHECK: [[GEP3:%.*]] = getelementptr <16 x i16>, ptr addrspace(1) [[GEP]], i32 3
; CHECK: store <16 x i16> zeroinitializer, ptr addrspace(1) [[GEP3]], align 16
  %gep = getelementptr inbounds %"class::buf", ptr addrspace(1) %0, i64 0
  call void @llvm.memset.p1.i64(ptr addrspace(1) align 16 %gep, i8 0, i64 160, i1 false)
  ret void
}


define void @kernel_alloca() {
entry:
; CHECK-LABEL: define void @kernel_alloca
; CHECK: [[BUF:%.*]] = alloca [32 x i32], align 2
; CHECK: [[GEP0:%.*]] = getelementptr <8 x i32>, ptr [[BUF]], i32 0
; CHECK: store <8 x i32> zeroinitializer, ptr [[GEP0]], align 4
; CHECK: [[GEP1:%.*]] = getelementptr <8 x i32>, ptr [[BUF]], i32 1
; CHECK: store <8 x i32> zeroinitializer, ptr [[GEP1]], align 4
; CHECK: [[GEP2:%.*]] = getelementptr <8 x i32>, ptr [[BUF]], i32 2
; CHECK: store <8 x i32> zeroinitializer, ptr [[GEP2]], align 4
  %buf = alloca [32 x i32], align 2
  call void @llvm.memset.p0.i64(ptr align 4 %buf, i8 0, i64 128, i1 false)
  ret void
}

define void @kernel_alloca_2() {
entry:
; CHECK-LABEL: define void @kernel_alloca_2
; CHECK: [[BUF:%.*]] = alloca [32 x i16], align 2
; CHECK: [[GEP0:%.*]] = getelementptr <16 x i16>, ptr [[BUF]], i32 0
; CHECK: store <16 x i16> zeroinitializer, ptr [[GEP0]], align 4
; CHECK: [[GEP1:%.*]] = getelementptr <16 x i16>, ptr [[BUF]], i32 1
; CHECK: store <16 x i16> zeroinitializer, ptr [[GEP1]], align 4
  %buf = alloca [32 x i16], align 2
  call void @llvm.memset.p0.i64(ptr align 4 %buf, i8 0, i64 64, i1 false)
  ret void
}
