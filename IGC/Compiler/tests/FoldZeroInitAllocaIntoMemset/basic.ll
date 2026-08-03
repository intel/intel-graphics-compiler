;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-17-plus
; RUN: igc_opt --opaque-pointers --igc-fold-zeroinit-alloca-into-memset -verify -S %s | FileCheck %s

; CHECK-NOT:  alloca
; CHECK-NOT:  memset
; CHECK-NOT:  bitcast
; CHECK:  call void @llvm.memset.p1.i64(ptr addrspace(1) align 4 %BUFFER, i8 0, i64 4000000, i1 false)

define spir_kernel void @test(ptr addrspace(1) %BUFFER) {
main:
  %alloca = alloca [4000000 x i8], align 4
  call void @llvm.memset.p0.i64(ptr align 1 %alloca, i8 0, i64 4000000, i1 false)
  br label %cpy
cpy:
  %bitcast = bitcast ptr %alloca to ptr
  call void @llvm.memcpy.p1.p0.i64(ptr addrspace(1) align 4 %BUFFER, ptr align 4 %bitcast, i64 4000000, i1 false)
  ret void
}

; CHECK:  alloca [5000000 x i8], align 4
; CHECK:  call void @llvm.memset.p0.i64(ptr align 1 %alloca, i8 1, i64 5000000
; CHECK:  bitcast
; CHECK:  call void @llvm.memcpy.p1.p0.i64(ptr addrspace(1) align 4 %BUFFER, ptr align 4 %bitcast, i64 5000000, i1 false)

define spir_kernel void @test_nonzero_shouldnt_be_removed(ptr addrspace(1) %BUFFER) {
main:
  %alloca = alloca [5000000 x i8], align 4
  call void @llvm.memset.p0.i64(ptr align 1 %alloca, i8 1, i64 5000000, i1 false)
  br label %cpy
cpy:
  %bitcast = bitcast ptr %alloca to ptr
  call void @llvm.memcpy.p1.p0.i64(ptr addrspace(1) align 4 %BUFFER, ptr align 4 %bitcast, i64 5000000, i1 false)
  ret void
}

; CHECK-NOT:  alloca
; CHECK-NOT:  memset
; CHECK-NOT:  bitcast
; CHECK:  call void @llvm.memset.p1.i64(ptr addrspace(1) align 4 %BUFFER, i8 0, i64 6000000, i1 false)
; CHECK:  call void @llvm.memset.p1.i64(ptr addrspace(1) align 4 %BUFFER, i8 0, i64 4, i1 false)

define spir_kernel void @test_multiple(ptr addrspace(1) %BUFFER) {
main:
  %alloca = alloca [6000000 x i8], align 4
  %alloca2 = alloca [4 x i8], align 4
  call void @llvm.memset.p0.i64(ptr align 1 %alloca, i8 0, i64 6000000, i1 false)
  call void @llvm.memset.p0.i64(ptr align 1 %alloca2, i8 0, i64 4, i1 false)
  br label %cpy
cpy:
  %bitcast = bitcast ptr %alloca to ptr
  call void @llvm.memcpy.p1.p0.i64(ptr addrspace(1) align 4 %BUFFER, ptr align 4 %bitcast, i64 6000000, i1 false)
  %bitcast2 = bitcast ptr %alloca2 to ptr
  call void @llvm.memcpy.p1.p0.i64(ptr addrspace(1) align 4 %BUFFER, ptr align 4 %bitcast2, i64 4, i1 false)
  ret void
}

; CHECK-NOT:  alloca
; CHECK-NOT:  memset
; CHECK-NOT:  bitcast
; CHECK:  call void @llvm.memset.p1.i64(ptr addrspace(1) align 2 %BUFFER, i8 0, i64 7000000, i1 false)

define spir_kernel void @test_different_alignment(ptr addrspace(1) %BUFFER) {
main:
  %alloca = alloca [7000000 x i8], align 2
  call void @llvm.memset.p0.i64(ptr align 1 %alloca, i8 0, i64 7000000, i1 false)
  br label %cpy
cpy:
  %bitcast = bitcast ptr %alloca to ptr
  call void @llvm.memcpy.p1.p0.i64(ptr addrspace(1) align 2 %BUFFER, ptr align 4 %bitcast, i64 7000000, i1 false)
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p1.p0.i64(ptr addrspace(1) writeonly, ptr readonly, i64, i1 immarg)

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr writeonly, i8, i64, i1 immarg)
