;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-custom-safe-opt -igc-gen-specific-pattern -dce -S < %s | FileCheck %s

; Test that the ptrtoint/addrspacecast/inttoptr fold works correctly in typed
; pointer mode.

target datalayout = "e-p:64:64-p0:64:64-p1:64:64-p3:32:32-p4:64:64-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"

define i64 @test_fold_typed_ptrs(i64 %x) {
; CHECK-LABEL: @test_fold_typed_ptrs
; CHECK: ret i64 %x
;
  %i2p = inttoptr i64 %x to i32*
  %asc = addrspacecast i32* %i2p to i32 addrspace(1)*
  %p2i = ptrtoint i32 addrspace(1)* %asc to i64
  ret i64 %p2i
}

define i64 @test_fold_no_addrspacecast_typed(i64 %x) {
; CHECK-LABEL: @test_fold_no_addrspacecast_typed
; CHECK: ret i64 %x
;
  %i2p = inttoptr i64 %x to i64*
  %p2i = ptrtoint i64* %i2p to i64
  ret i64 %p2i
}

; Do NOT fold when generic address space (AS 4) is the destination.
define i64 @test_no_fold_to_generic_typed(i64 %x) {
; CHECK-LABEL: @test_no_fold_to_generic_typed
; CHECK: inttoptr
; CHECK: addrspacecast
; CHECK: ptrtoint
;
  %i2p = inttoptr i64 %x to i32 addrspace(1)*
  %asc = addrspacecast i32 addrspace(1)* %i2p to i32 addrspace(4)*
  %p2i = ptrtoint i32 addrspace(4)* %asc to i64
  ret i64 %p2i
}

; Do NOT fold when generic address space (AS 4) is the source.
define i64 @test_no_fold_from_generic_typed(i64 %x) {
; CHECK-LABEL: @test_no_fold_from_generic_typed
; CHECK: inttoptr
; CHECK: addrspacecast
; CHECK: ptrtoint
;
  %i2p = inttoptr i64 %x to i32 addrspace(4)*
  %asc = addrspacecast i32 addrspace(4)* %i2p to i32 addrspace(1)*
  %p2i = ptrtoint i32 addrspace(1)* %asc to i64
  ret i64 %p2i
}

; Do NOT fold when sizes differ.
define i64 @test_no_fold_different_sizes_typed(i64 %x) {
; CHECK-LABEL: @test_no_fold_different_sizes_typed
; CHECK: inttoptr
; CHECK: addrspacecast
; CHECK: ptrtoint
;
  %i2p = inttoptr i64 %x to i32 addrspace(1)*
  %asc = addrspacecast i32 addrspace(1)* %i2p to i32 addrspace(3)*
  %p2i = ptrtoint i32 addrspace(3)* %asc to i64
  ret i64 %p2i
}
