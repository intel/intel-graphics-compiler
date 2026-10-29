;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;

; REQUIRES: regkeys
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

; This test verifies if the WIAnalysis correctly treats localID argument of sub_group_broadcast
; as non-uniform if it comes directly from a load instruction.

; Function Attrs: convergent nounwind
define spir_func void @test_local_id_from_load(i32 %val, i32 %slid, i32 addrspace(1)* %dst, i64 %gid) {
entry:
  %a = alloca i32, align 4
  store i32 %slid, i32* %a, align 4
  %localID = load i32, i32* %a, align 4
  %simdBroadcast = call i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32 %val, i32 %localID, i32 0)
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %dst, i64 %gid
  store i32 %simdBroadcast, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; CHECK: block: entry function: test_local_id_from_load
; CHECK-TYPED: N: {{.*}}  %localID = load i32, i32* %a, align 4
; CHECK-OPAQUE: N: {{.*}}  %localID = load i32, ptr %a, align 4

declare i32 @llvm.genx.GenISA.WaveBroadcast.i32(i32, i32, i32)
