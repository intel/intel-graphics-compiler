;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-emu64ops -S < %s | FileCheck %s

; Test Emu64Ops optimization that avoids redundant conversions between pointers/pairs
; when the following pattern occurs:
; pair.to.ptr(lo, hi) -> addrspacecast -> ptrtoint -> (lo, hi)
; The optimization reuses the original (lo, hi) values directly, eliminating the
; redundant ptr.to.pair call.

define void @test_roundtrip_elimination(i32 %lo, i32 %hi) {
; CHECK-LABEL: @test_roundtrip_elimination
;
; The pair.to.ptr and addrspacecast remain (they may be used elsewhere or will be
; eliminated by DCE)
; CHECK: call ptr addrspace(4) @llvm.genx.GenISA.pair.to.ptr.p4.i32(i32 %lo, i32 %hi)
; CHECK: addrspacecast

; Reuse the original %lo directly, no ptr.to.pair call
; CHECK: call void @use.i32.i32(i32 %lo, i32 [[HI:%.*]])

; Verify ptr.to.pair was NOT emitted (that would be the redundant roundtrip)
; CHECK-NOT: ptr.to.pair
entry:
  ; Start with lo and hi values
  ; Convert to pointer representation
  %ptr = call ptr addrspace(4) @llvm.genx.GenISA.pair.to.ptr.p4.i32(i32 %lo, i32 %hi)

  ; Cast to different address space
  %cast = addrspacecast ptr addrspace(4) %ptr to ptr addrspace(1)

  ; Convert back to integer (triggers Emu64Ops expansion)
  ; Without optimization, would call ptr.to.pair(%cast) to get (lo, hi)
  ; With optimization, recognizes we already have the original (lo, hi)
  %i64_val = ptrtoint ptr addrspace(1) %cast to i64

  ; Extract the two i32 parts from the i64
  %extracted_lo = trunc i64 %i64_val to i32
  %shift = lshr i64 %i64_val, 32
  %extracted_hi = trunc i64 %shift to i32

  ; Use the values
  call void @use.i32.i32(i32 %extracted_lo, i32 %extracted_hi)
  ret void
}

declare ptr addrspace(4) @llvm.genx.GenISA.pair.to.ptr.p4.i32(i32, i32)
declare void @use.i32.i32(i32, i32)

!igc.functions = !{!0}
!0 = !{ptr @test_roundtrip_elimination, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
