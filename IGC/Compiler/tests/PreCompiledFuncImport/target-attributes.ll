;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=65 --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s

; Host target attributes must not prevent inlining the emulation libraries.
; End-to-end coverage: ocloc_tests/DebugInfo/debug-frame.cl.

; CHECK-LABEL: define i64 @divide(
; CHECK: call spir_func i64 @__igcbuiltin_u64_udiv_sp(i64 %a, i64 %b)
; CHECK: ret i64
define i64 @divide(i64 %a, i64 %b) {
  %result = udiv i64 %a, %b
  ret i64 %result
}

; CHECK-DAG: define internal spir_func i64 @__igcbuiltin_u64_udiv_sp({{.*}}) #[[I64:[0-9]+]]
; CHECK-DAG: define internal spir_func i32 @precompiled_u32divrem_sp({{.*}}) #[[I32:[0-9]+]]
; CHECK: attributes #[[I64]] = { alwaysinline
; CHECK-NOT: "target-cpu"
; CHECK-NOT: "target-features"
; CHECK: }
; CHECK: attributes #[[I32]] = { alwaysinline
; CHECK-NOT: "target-cpu"
; CHECK-NOT: "target-features"
; CHECK: }
