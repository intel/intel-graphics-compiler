;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --platformdg2 --opaque-pointers --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s --check-prefix=CHECK-DG2
; RUN: igc_opt --platformbmg --opaque-pointers --igc-PartialEmuI64Ops -S < %s 2>&1 | FileCheck %s --check-prefix=CHECK-BMG
; ------------------------------------------------
; PartialEmuI64Ops : OR to ADD conversion when operands have no common bits and QWadd is supported
; ------------------------------------------------

; 64-bit OR with disjoint known bits likely originated from add i64 before InstCombine simplified it
; 64-bit OR requires emulation on platforms that support 64-bit ADD.
; Test that InstCombine optimization is reverted back to add i64 on platforms that support 64-bit add.

define void @test_or_no_common_bits() {
; On BMG (has QWAdd): OR with no common bits -> ADD
; CHECK-BMG-LABEL: @test_or_no_common_bits(
; CHECK-BMG-NOT: or i64
; CHECK-BMG: [[ADD:%.*]] = add i64 %addr_int, 108
; CHECK-BMG: call void @use.i64(i64 [[ADD]])

; On DG2 (no QWdd): OR stays as OR, then gets emulated (see basic.ll)
; CHECK-DG2-LABEL: @test_or_no_common_bits(
; CHECK-DG2-NOT: add i64
;
  %addr = call align 256 ptr @llvm.genx.GenISA.InlinedData.p2i8(i32 2)
  %addr_int = ptrtoint ptr %addr to i64
  %sum = or i64 %addr_int, 108
  call void @use.i64(i64 %sum)
  ret void
}

declare ptr @llvm.genx.GenISA.InlinedData.p2i8(i32)

declare void @use.i64(i64)

!igc.functions = !{!0}

!0 = !{void ()* @test_or_no_common_bits, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
