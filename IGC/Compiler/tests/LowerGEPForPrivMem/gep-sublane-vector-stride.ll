;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -platformCri -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerGEPForPrivMem
; ------------------------------------------------
; A GEP striding over <2 x i8> on i32 lanes steps 2 bytes, half a lane, per index.

; gep <2 x i8>, a+8, 2 is byte 12, lane 3.
define void @const_idx(ptr addrspace(1) %out) {
; CHECK-LABEL: @const_idx(
; CHECK:       alloca <8 x i32>
; CHECK:       store <8 x i32>
; CHECK:       [[V:%.*]] = load <8 x i32>
; CHECK:       extractelement <8 x i32> [[V]], i32 3
  %a = alloca [8 x i32], align 4
  %a3 = getelementptr [8 x i32], ptr %a, i32 0, i32 3
  store i32 3, ptr %a3, align 4
  %a2 = getelementptr [8 x i32], ptr %a, i32 0, i32 2
  %p = getelementptr <2 x i8>, ptr %a2, i32 2
  %x = load <4 x i8>, ptr %p, align 4
  %xi = bitcast <4 x i8> %x to i32
  store i32 %xi, ptr addrspace(1) %out, align 4
  ret void
}

; gep <2 x i8>, a+8, 2*k is lane 2 + k.
define void @dyn_idx(ptr addrspace(1) %out, i32 %n) {
; CHECK-LABEL: @dyn_idx(
; CHECK:       alloca <8 x i32>
; CHECK:       store <8 x i32>
; CHECK:       [[K:%.*]] = and i32 %n, 3
; CHECK:       [[K2:%.*]] = shl i32 [[K]], 1
; CHECK:       [[B:%.*]] = mul i32 [[K2]], 2
; CHECK:       [[L:%.*]] = sdiv i32 [[B]], 4
; CHECK:       [[I:%.*]] = add i32 2, [[L]]
; CHECK:       [[V:%.*]] = load <8 x i32>
; CHECK:       [[I0:%.*]] = add i32 [[I]], 0
; CHECK:       extractelement <8 x i32> [[V]], i32 [[I0]]
  %a = alloca [8 x i32], align 4
  %a3 = getelementptr [8 x i32], ptr %a, i32 0, i32 3
  store i32 3, ptr %a3, align 4
  %a2 = getelementptr [8 x i32], ptr %a, i32 0, i32 2
  %k = and i32 %n, 3
  %k2 = shl i32 %k, 1
  %p = getelementptr <2 x i8>, ptr %a2, i32 %k2
  %x = load <4 x i8>, ptr %p, align 4
  %xi = bitcast <4 x i8> %x to i32
  store i32 %xi, ptr addrspace(1) %out, align 4
  ret void
}

; An odd k lands mid-lane, so the alloca is not promoted.
define void @unaligned_dyn_idx(ptr addrspace(1) %out, i32 %n) {
; CHECK-LABEL: @unaligned_dyn_idx(
; CHECK:       alloca [8 x i32]
; CHECK-NOT:   alloca <8 x i32>
  %a = alloca [8 x i32], align 4
  %a3 = getelementptr [8 x i32], ptr %a, i32 0, i32 3
  store i32 3, ptr %a3, align 4
  %a2 = getelementptr [8 x i32], ptr %a, i32 0, i32 2
  %k = and i32 %n, 3
  %p = getelementptr <2 x i8>, ptr %a2, i32 %k
  %x = load <4 x i8>, ptr %p, align 4
  %xi = bitcast <4 x i8> %x to i32
  store i32 %xi, ptr addrspace(1) %out, align 4
  ret void
}

!igc.functions = !{!0, !3, !4}

!0 = !{ptr @const_idx, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @dyn_idx, !1}
!4 = !{ptr @unaligned_dyn_idx, !1}
