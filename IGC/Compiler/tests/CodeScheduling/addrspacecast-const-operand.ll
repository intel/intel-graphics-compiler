;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers --igc-code-scheduling -S %s | FileCheck %s

; Checks that an addrspacecast of a constant doesn't make CodeScheduling query ConstantData uses.
; The DPAS makes CodeScheduling process this block; blocks without DPAS are skipped.

define void @test(ptr addrspace(3) %p, ptr addrspace(1) %out, <8 x i16> %a, <8 x i32> %b) {
; CHECK-LABEL: @test(
; CHECK: [[CAST:%.*]] = addrspacecast ptr addrspace(4) null to ptr addrspace(3)
; CHECK: icmp eq ptr addrspace(3) %p, [[CAST]]
entry:
  %cast = addrspacecast ptr addrspace(4) null to ptr addrspace(3)
  %cmp = icmp eq ptr addrspace(3) %p, %cast
  %z = zext i1 %cmp to i32
  store i32 %z, ptr addrspace(1) %out
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %out
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
