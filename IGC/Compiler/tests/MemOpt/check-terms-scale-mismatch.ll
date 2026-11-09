;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; SymbolicPointer::checkTerms() must not merge loads whose differing terms have
; different scales: p[x + 3 - y] and p[2 * (x + 2 - y)] are 4 * (x - y + 1) bytes
; apart, not a constant 4.
;
; RUN: igc_opt --opaque-pointers --platformbmg --inputocl %s -S -o - --basic-aa -igc-memopt | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: @scale_mismatch
; CHECK-NOT: load <2 x i32>
; CHECK: %a = load i32, ptr addrspace(1) %pa, align 4
; CHECK: %b = load i32, ptr addrspace(1) %pb, align 4
; CHECK: ret void
define spir_kernel void @scale_mismatch(ptr addrspace(1) %p, ptr addrspace(1) %out, i32 %x, i32 %y) {
entry:
  %add0 = add nsw i32 %x, 3
  %sub0 = sub nsw i32 %add0, %y
  %idx0 = sext i32 %sub0 to i64
  %pa = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %idx0
  %a = load i32, ptr addrspace(1) %pa, align 4
  %add1 = add nsw i32 %x, 2
  %sub1 = sub nsw i32 %add1, %y
  %mul = shl nsw i32 %sub1, 1
  %idx1 = sext i32 %mul to i64
  %pb = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %idx1
  %b = load i32, ptr addrspace(1) %pb, align 4
  %o1 = getelementptr inbounds i32, ptr addrspace(1) %out, i64 1
  store i32 %a, ptr addrspace(1) %out, align 4
  store i32 %b, ptr addrspace(1) %o1, align 4
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @scale_mismatch, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
