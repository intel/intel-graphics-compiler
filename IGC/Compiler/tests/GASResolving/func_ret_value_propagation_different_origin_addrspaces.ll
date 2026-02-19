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

; CHECK: define internal spir_func i32 addrspace(3)* @f0() {
; CHECK: %[[ITP:.*]] = inttoptr i64 2271560481 to i32 addrspace(3)*
; CHECK-NOT: addrspacecast i32 addrspace(3)* %{{.*}} to i32 addrspace(4)*
; CHECK: ret i32 addrspace(3)* %[[ITP]]
define internal spir_func i32 addrspace(4)* @f0() {
  %1 = inttoptr i64 2271560481 to i32 addrspace(3)*
  %2 = addrspacecast i32 addrspace(3)* %1 to i32 addrspace(4)*
  ret i32 addrspace(4)* %2
}

; CHECK: define internal spir_func i32 addrspace(1)* @f1() {
; CHECK: %[[ITP:.*]] = inttoptr i64 2271560481 to i32 addrspace(1)*
; CHECK-NOT: addrspacecast i32 addrspace(1)* %{{.*}} to i32 addrspace(4)*
; CHECK: ret i32 addrspace(1)* %[[ITP]]
define internal spir_func i32 addrspace(4)* @f1() {
  %1 = inttoptr i64 2271560481 to i32 addrspace(1)*
  %2 = addrspacecast i32 addrspace(1)* %1 to i32 addrspace(4)*
  ret i32 addrspace(4)* %2
}

; Don't expect @foo return value propagation, since it may return local or global pointer.
; CHECK: define internal spir_func i32 addrspace(4)* @foo(i32 %gid) {
define internal spir_func i32 addrspace(4)* @foo(i32 %gid) {
  %rem = srem i32 %gid, 2
  %zero = icmp ne i32 %rem, 0
  br i1 %zero, label %if.zero, label %if.nonzero

if.zero:
  ; CHECK: %[[C0:.*]] = call spir_func i32 addrspace(3)* @f0()
  %call = call spir_func i32 addrspace(4)* @f0()
  ; CHECK: %[[AS0:.*]] = addrspacecast i32 addrspace(3)* %[[C0]] to i32 addrspace(4)*
  ; CHECK: ret i32 addrspace(4)* %[[AS0]]
  ret i32 addrspace(4)* %call

if.nonzero:
  ; CHECK: %[[C1:.*]] = call spir_func i32 addrspace(1)* @f1()
  %call1 = call spir_func i32 addrspace(4)* @f1()
  ; CHECK: %[[AS1:.*]] = addrspacecast i32 addrspace(1)* %[[C1]] to i32 addrspace(4)*
  ; CHECK: ret i32 addrspace(4)* %[[AS1]]
  ret i32 addrspace(4)* %call1
}

define spir_kernel void @kernel() {
entry:
  %0 = call spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()
  %1 = extractelement <3 x i64> %0, i32 0
  %gid = trunc i64 %1 to i32
  ; CHECK: %[[C2:.*]] = call spir_func i32 addrspace(4)* @foo(i32 %gid)
  %call1 = call spir_func i32 addrspace(4)* @foo(i32 %gid)
  ; CHECK: %[[GEP:.*]] = getelementptr inbounds i32, i32 addrspace(4)* %[[C2]], i64 0
  %arrayidx = getelementptr inbounds i32, i32 addrspace(4)* %call1, i64 0
  ; CHECK: store i32 5, i32 addrspace(4)* %[[GEP]], align 4
  store i32 5, i32 addrspace(4)* %arrayidx, align 4
  ret void
}

declare spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()

!igc.functions = !{!0, !3, !4, !5}

!0 = !{void ()* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
; CHECK: !{i32 addrspace(4)* (i32)* @foo, !1}
!3 = !{i32 addrspace(4)* (i32)* @foo, !1}
; CHECK: !{i32 addrspace(3)* ()* @f0, !1}
!4 = !{i32 addrspace(4)* ()* @f0, !1}
; CHECK: !{i32 addrspace(1)* ()* @f1, !1}
!5 = !{i32 addrspace(4)* ()* @f1, !1}
