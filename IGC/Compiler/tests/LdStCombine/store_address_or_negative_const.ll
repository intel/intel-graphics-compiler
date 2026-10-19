;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers %s -S -igc-ldstcombine -regkey=EnableLdStCombine=1 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

 ; Symbolic evaluation of 'or' with a negative (sign bit set) constant.
 ;   %i = or i8 %x, -128
 ;   %z = zext i8 %i to i64
 ; must not be treated as 'add %x, -128', as zext(%i) == zext(%x) + 128.

 ; Unknown sign bit of %k: must not assert on APInt construction.
 ; p[zext(i)] and p[zext(i) + 1] are adjacent and are combined.
 ; CHECK-LABEL: define spir_kernel void @test_or_unknown_bit
 ; CHECK: store <2 x i32> <i32 1, i32 2>,
 ; CHECK: ret void

 ; Known zero sign bit of %x: p[x + 128] and p[x - 127] are not adjacent,
 ; so the stores must not be combined.
 ; CHECK-LABEL: define spir_kernel void @test_or_known_zero_bit
 ; CHECK-NOT: store <2 x i32>
 ; CHECK: store i32 1,
 ; CHECK: store i32 2,
 ; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_or_unknown_bit(ptr addrspace(1) %p, i8 %k) {
entry:
  %i = or i8 %k, -128
  %z0 = zext i8 %i to i64
  %a0 = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %z0
  store i32 1, ptr addrspace(1) %a0, align 4
  %z1 = zext i8 %i to i32
  %add = add nuw nsw i32 %z1, 1
  %z2 = zext i32 %add to i64
  %a1 = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %z2
  store i32 2, ptr addrspace(1) %a1, align 4
  ret void
}

define spir_kernel void @test_or_known_zero_bit(ptr addrspace(1) %p, i8 %k) {
entry:
  %x = lshr i8 %k, 1
  %i = or i8 %x, -128
  %z0 = zext i8 %i to i64
  %a0 = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %z0
  store i32 1, ptr addrspace(1) %a0, align 4
  %zx = zext i8 %x to i64
  %sub = add nsw i64 %zx, -127
  %a1 = getelementptr inbounds i32, ptr addrspace(1) %p, i64 %sub
  store i32 2, ptr addrspace(1) %a1, align 4
  ret void
}
