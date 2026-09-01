;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Integer constants wider than 64 bits must not reach getZExtValue(), which
; asserts on "Too many bits for uint64_t".
;
; RUN: igc_opt --opaque-pointers -igc-fix-alignment -S < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @test_i128_add(i64 %n) {
; CHECK-LABEL: @test_i128_add(
; CHECK:    add nsw i128
; CHECK:    icmp ugt i128
; CHECK:    ret void

entry:
  %wide = zext i64 %n to i128
  %dec = add nsw i128 %wide, -1
  %ovf = icmp ugt i128 %dec, 18446744073709551615
  %z = zext i1 %ovf to i64
  store volatile i64 %z, ptr addrspace(3) null, align 8
  ret void
}

; Low 64 bits all zero: alignment saturates rather than tripping the truncation.
define spir_kernel void @test_i128_high_bits_only(ptr addrspace(1) %p) {
; CHECK-LABEL: @test_i128_high_bits_only(
; CHECK:    add i128
; CHECK:    ret void

entry:
  %pi = ptrtoint ptr addrspace(1) %p to i128
  %sum = add i128 %pi, 18446744073709551616
  %lo = trunc i128 %sum to i64
  store volatile i64 %lo, ptr addrspace(3) null, align 8
  ret void
}
