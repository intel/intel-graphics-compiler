;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Verify that when a loaded pointer is used as an indirect call callee (not a
; data operand), the source alloca is NOT considered a lifetime-leaking use.
; As a result alloca1 and alloca2 have non-overlapping lifetimes and can be
; merged into a single alloca.

; RUN: igc_opt --opaque-pointers -S --igc-ocl-merge-allocas %s | FileCheck %s

; CHECK: alloca ptr
; CHECK-NOT: alloca ptr
define void @test() #0 {
  %alloca1 = alloca ptr
  %alloca2 = alloca ptr
  %fp = load ptr, ptr %alloca1
  call void %fp()
  store ptr null, ptr %alloca2
  ret void
}

attributes #0 = { nounwind readnone }
