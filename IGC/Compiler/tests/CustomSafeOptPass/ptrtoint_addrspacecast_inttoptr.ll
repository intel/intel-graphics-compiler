;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-custom-safe-opt -igc-gen-specific-pattern -dce -S < %s | FileCheck %s

; Test that CustomSafeOptPass correctly simplifies redundant pointer conversion
; chains. This pattern is common with opaque pointers when different passes
; emit inttoptr -> addrspacecast -> ptrtoint sequences. The fold occurs when
; pointer sizes and result types match.

target datalayout = "e-p:64:64-p0:64:64-p1:64:64-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"

define i64 @test_ptrtoint_addrspacecast_inttoptr_i64(i64 %x) {
; CHECK-LABEL: @test_ptrtoint_addrspacecast_inttoptr_i64
; CHECK: ret i64 %x
;
  %i2p = inttoptr i64 %x to ptr
  %asc = addrspacecast ptr %i2p to ptr addrspace(1)
  %p2i = ptrtoint ptr addrspace(1) %asc to i64
  ret i64 %p2i
}

define i64 @test_ptrtoint_inttoptr_no_addrspacecast(i64 %x) {
; CHECK-LABEL: @test_ptrtoint_inttoptr_no_addrspacecast
; CHECK: ret i64 %x
;
  %i2p = inttoptr i64 %x to ptr
  %p2i = ptrtoint ptr %i2p to i64
  ret i64 %p2i
}

define i64 @test_ptrtoint_addrspacecast_inttoptr_with_use(i64 %x) {
; CHECK-LABEL: @test_ptrtoint_addrspacecast_inttoptr_with_use
; CHECK:    [[ADD:%.*]] = add i64 %x, 42
; CHECK:    ret i64 [[ADD]]
;
  %i2p = inttoptr i64 %x to ptr
  %asc = addrspacecast ptr %i2p to ptr addrspace(1)
  %p2i = ptrtoint ptr addrspace(1) %asc to i64
  %result = add i64 %p2i, 42
  ret i64 %result
}

; Do NOT fold when generic address space (AS 4) is the destination.
define i64 @test_no_fold_to_generic(i64 %x) {
; CHECK-LABEL: @test_no_fold_to_generic
; CHECK: inttoptr
; CHECK: addrspacecast
; CHECK: ptrtoint
;
  %i2p = inttoptr i64 %x to ptr addrspace(1)
  %asc = addrspacecast ptr addrspace(1) %i2p to ptr addrspace(4)
  %p2i = ptrtoint ptr addrspace(4) %asc to i64
  ret i64 %p2i
}

; Do NOT fold when generic address space (AS 4) is the source.
define i64 @test_no_fold_from_generic(i64 %x) {
; CHECK-LABEL: @test_no_fold_from_generic
; CHECK: inttoptr
; CHECK: addrspacecast
; CHECK: ptrtoint
;
  %i2p = inttoptr i64 %x to ptr addrspace(4)
  %asc = addrspacecast ptr addrspace(4) %i2p to ptr addrspace(1)
  %p2i = ptrtoint ptr addrspace(1) %asc to i64
  ret i64 %p2i
}
