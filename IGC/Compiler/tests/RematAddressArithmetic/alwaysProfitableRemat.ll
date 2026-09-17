;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers %s -S -igc-clone-address-arithmetic --regkey=RematFlowThreshold=1 --regkey=RematRPELimit=0 --dce | FileCheck %s

; A binary operator with a constant operand is "always profitable" to
; rematerialize: cloning it keeps only its non-constant operand live. Even with
; RematFlowThreshold=1 (so the use-count heuristic rejects everything), the
; "or i32 %base, C" instructions are still cloned next to each icmp use, while
; the base "or i32 %s, %m" (no constant operand) stays as the shared origin.

define spir_kernel void @test(ptr addrspace(1) %p, i32 %n, <8 x i32> %r0, i16 %lid) {
entry:
  %tid = extractelement <8 x i32> %r0, i64 1
  %s = shl i32 %tid, 7
  %z = zext i16 %lid to i32
  %m = and i32 %z, 120
  %base = or i32 %s, %m
  %o1 = or i32 %base, 1
  %o2 = or i32 %base, 2
  %o3 = or i32 %base, 3
  br label %use

; CHECK-LABEL: entry:
; CHECK: %base = or i32 %s, %m
; CHECK: br label %use

; CHECK-LABEL: use:
; CHECK: [[C0:%cloned_.*]] = icmp slt i32 %base, %n
; CHECK: [[O1:%remat.*]] = or i32 %base, 1
; CHECK: [[C1:%cloned_.*]] = icmp slt i32 [[O1]], %n
; CHECK: and i1 [[C0]], [[C1]]
; CHECK: [[O2:%remat.*]] = or i32 %base, 2
; CHECK: [[C2:%cloned_.*]] = icmp slt i32 [[O2]], %n
; CHECK: [[O3:%remat.*]] = or i32 %base, 3
; CHECK: [[C3:%cloned_.*]] = icmp slt i32 [[O3]], %n

use:
  %c0 = icmp slt i32 %base, %n
  %c1 = icmp slt i32 %o1, %n
  %c2 = icmp slt i32 %o2, %n
  %c3 = icmp slt i32 %o3, %n
  %a0 = and i1 %c0, %c1
  %a1 = and i1 %a0, %c2
  %a2 = and i1 %a1, %c3
  br i1 %a2, label %then, label %exit

then:
  store i32 %n, ptr addrspace(1) %p, align 4
  br label %exit

exit:
  ret void
}

!igc.functions = !{}
