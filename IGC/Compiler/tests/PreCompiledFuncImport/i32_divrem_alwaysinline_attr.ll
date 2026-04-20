;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=32 --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s
;
; Verify that the i32 div/rem emulation functions (precompiled_u32divrem,
; precompiled_s32divrem) carry 'alwaysinline' as a function-level attribute,
; not as a return attribute.

; CHECK-NOT: declare alwaysinline

; --- unsigned division triggers precompiled_u32divrem ---
; CHECK-LABEL: @test_udiv(
; CHECK:       call i32 @precompiled_u32divrem
define i32 @test_udiv(i32 %a, i32 %b) #0 {
  %r = udiv i32 %a, %b
  ret i32 %r
}

; --- signed division triggers precompiled_s32divrem ---
; CHECK-LABEL: @test_sdiv(
; CHECK:       call i32 @precompiled_s32divrem
define i32 @test_sdiv(i32 %a, i32 %b) #0 {
  %r = sdiv i32 %a, %b
  ret i32 %r
}

; Verify alwaysinline is a function-level attribute (in attributes group)
; CHECK-DAG: define {{.*}} @precompiled_u32divrem({{.*}}) #[[ATTR:[0-9]+]]
; CHECK-DAG: define {{.*}} @precompiled_s32divrem({{.*}}) #[[ATTR]]
; CHECK: attributes #[[ATTR]] = {{{.*}}alwaysinline{{.*}}}

attributes #0 = { nounwind }
