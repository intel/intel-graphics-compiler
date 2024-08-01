;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus, pvc-supported
;
; RUN: igc_opt %s -S -o - -igc-custom-safe-opt -platform pvc | FileCheck %s

define spir_kernel void @block_prefetch_d16.1x16x8nn(i64 %val1, i32 %val2) {
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ret void
}

define spir_kernel void @block_prefetch_d16.2x16x8nn(i64 %val1, i32 %val2) {
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 16, i32 16, i32 8, i32 2, i1 false, i1 false, i32 4)
  ;
  ; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ret void
}

define spir_kernel void @block_prefetch_d16.4x8x8nn(i64 %val1, i32 %val2) {
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 16, i32 8, i32 8, i32 4, i1 false, i1 false, i32 4)
  ;
  ; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ret void
}

define spir_kernel void @block_prefetch_d8.4x64x8nn(i64 %val1, i32 %val2) {
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 8, i32 64, i32 8, i32 4, i1 false, i1 false, i32 4)
  ;
  ; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 8, i32 256, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ret void
}

define spir_kernel void @block_prefetch_d8.4x16x8nn(i64 %val1, i32 %val2) {
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 8, i32 16, i32 8, i32 4, i1 false, i1 false, i32 4)
  ;
  ; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 8, i32 64, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ret void
}

define spir_kernel void @block_prefetch_d32.2x16x8nn(i64 %val1, i32 %val2) {
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 32, i32 16, i32 8, i32 2, i1 false, i1 false, i32 4)
  ;
  ; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 32, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ret void
}

define spir_kernel void @block_prefetch_d32.2x32x8nn(i64 %val1, i32 %val2) {
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 32, i32 32, i32 8, i32 2, i1 false, i1 false, i32 4)
  ;
  ; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 32, i32 64, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ret void
}

define spir_kernel void @block_prefetch_d64.4x8x8nn(i64 %val1, i32 %val2) {
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 64, i32 8, i32 8, i32 4, i1 false, i1 false, i32 4)
  ;
  ; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 64, i32 8, i32 8, i32 4, i1 false, i1 false, i32 4)
  ;
  ret void
}

define spir_kernel void @block_prefetch_d64.1x8x8nn(i64 %val1, i32 %val2) {
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 64, i32 8, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 %val1, i32 8191, i32 4095, i32 8191, i32 0, i32 %val2, i32 64, i32 8, i32 8, i32 1, i1 false, i1 false, i32 4)
  ;
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
