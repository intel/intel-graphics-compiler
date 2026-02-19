;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-gas-ret-value-propagator | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

; Expecting that ResolveGAS won't change any addrspace in this llvm module.
; The test is designed to make sure that GASRetValuePropagator won't crash.

; CHECK: define internal spir_func i32 addrspace(4)* @foo(i32 %gid) {
define internal spir_func i32 addrspace(4)* @foo(i32 %gid) {
  %rem = srem i32 %gid, 2
  %zero = icmp ne i32 %rem, 0
  br i1 %zero, label %if.zero, label %if.nonzero

if.zero:
  ; CHECK ret i32 addrspace(4)* null
  ret i32 addrspace(4)* null

if.nonzero:
  ; CHECK ret i32 addrspace(4)* null
  ret i32 addrspace(4)* null
}

define spir_kernel void @kernel() {
entry:
  %0 = call spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()
  %1 = extractelement <3 x i64> %0, i32 0
  %gid = trunc i64 %1 to i32
  ; CHECK: %[[C2:.*]] = call spir_func i32 addrspace(4)* @foo(i32 %gid)
  %call1 = call spir_func i32 addrspace(4)* @foo(i32 %gid)
  ret void
}

declare spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()

!igc.functions = !{!0, !3}

!0 = !{void ()* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
; CHECK: !{i32 addrspace(4)* (i32)* @foo, !1}
!3 = !{i32 addrspace(4)* (i32)* @foo, !1}
