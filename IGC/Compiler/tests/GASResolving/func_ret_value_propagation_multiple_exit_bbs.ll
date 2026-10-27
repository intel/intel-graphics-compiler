;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - -igc-gas-ret-value-propagator | FileCheck --check-prefixes=CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - -igc-gas-ret-value-propagator | FileCheck --check-prefixes=CHECK-TYPED %s %}

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

; CHECK-TYPED: define internal spir_func i32 addrspace(3)* @f0() {
; CHECK-OPAQUE: define internal spir_func ptr addrspace(3) @f0() {
; CHECK-TYPED: %[[ITP:.*]] = inttoptr i64 2271560481 to i32 addrspace(3)*
; CHECK-OPAQUE: %[[ITP:.*]] = inttoptr i64 2271560481 to ptr addrspace(3)
; CHECK-TYPED-NOT: addrspacecast i32 addrspace(3)* %{{.*}} to i32 addrspace(4)*
; CHECK-OPAQUE-NOT: addrspacecast ptr addrspace(3) %{{.*}} to ptr addrspace(4)
; CHECK-TYPED: ret i32 addrspace(3)* %[[ITP]]
; CHECK-OPAQUE: ret ptr addrspace(3) %[[ITP]]
define internal spir_func i32 addrspace(4)* @f0() {
  %1 = inttoptr i64 2271560481 to i32 addrspace(3)*
  %2 = addrspacecast i32 addrspace(3)* %1 to i32 addrspace(4)*
  ret i32 addrspace(4)* %2
}

; CHECK-TYPED: define internal spir_func i32 addrspace(3)* @f1() {
; CHECK-OPAQUE: define internal spir_func ptr addrspace(3) @f1() {
; CHECK-TYPED: %[[ITP:.*]] = inttoptr i64 2271560481 to i32 addrspace(3)*
; CHECK-OPAQUE: %[[ITP:.*]] = inttoptr i64 2271560481 to ptr addrspace(3)
; CHECK-TYPED-NOT: addrspacecast i32 addrspace(3)* %{{.*}} to i32 addrspace(4)*
; CHECK-OPAQUE-NOT: addrspacecast ptr addrspace(3) %{{.*}} to ptr addrspace(4)
; CHECK-TYPED: ret i32 addrspace(3)* %[[ITP]]
; CHECK-OPAQUE: ret ptr addrspace(3) %[[ITP]]
define internal spir_func i32 addrspace(4)* @f1() {
  %1 = inttoptr i64 2271560481 to i32 addrspace(3)*
  %2 = addrspacecast i32 addrspace(3)* %1 to i32 addrspace(4)*
  ret i32 addrspace(4)* %2
}

; CHECK-TYPED: define internal spir_func i32 addrspace(3)* @foo(i32 %gid) {
; CHECK-OPAQUE: define internal spir_func ptr addrspace(3) @foo(i32 %gid) {
define internal spir_func i32 addrspace(4)* @foo(i32 %gid) {
  %rem = srem i32 %gid, 3
  %zero = icmp ne i32 %rem, 0
  %one = icmp ne i32 %rem, 1
  br i1 %zero, label %if.zero, label %nonzero

if.zero:
  ; CHECK-TYPED: %[[C0:.*]] = call spir_func i32 addrspace(3)* @f0()
  ; CHECK-OPAQUE: %[[C0:.*]] = call spir_func ptr addrspace(3) @f0()
  %call = call spir_func i32 addrspace(4)* @f0()
  ; CHECK-TYPED: ret i32 addrspace(3)* %[[C0]]
  ; CHECK-OPAQUE: ret ptr addrspace(3) %[[C0]]
  ret i32 addrspace(4)* %call

if.one:
  ; CHECK-TYPED: %[[C1:.*]] = call spir_func i32 addrspace(3)* @f1()
  ; CHECK-OPAQUE: %[[C1:.*]] = call spir_func ptr addrspace(3) @f1()
  %call1 = call spir_func i32 addrspace(4)* @f1()
  ; CHECK-TYPED: ret i32 addrspace(3)* %[[C1]]
  ; CHECK-OPAQUE: ret ptr addrspace(3) %[[C1]]
  ret i32 addrspace(4)* %call1

if.two:
  ; CHECK-TYPED: ret i32 addrspace(3)* null
  ; CHECK-OPAQUE: ret ptr addrspace(3) null
  ret i32 addrspace(4)* null

nonzero:
  br i1 %one, label %if.one, label %if.two
}

define spir_kernel void @kernel() {
entry:
  %0 = call spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()
  %1 = extractelement <3 x i64> %0, i32 0
  %gid = trunc i64 %1 to i32
  ; CHECK-TYPED: %[[C2:.*]] = call spir_func i32 addrspace(3)* @foo(i32 %gid)
  ; CHECK-OPAQUE: %[[C2:.*]] = call spir_func ptr addrspace(3) @foo(i32 %gid)
  %call1 = call spir_func i32 addrspace(4)* @foo(i32 %gid)
  ; CHECK-TYPED: %[[GEP:.*]] = getelementptr inbounds i32, i32 addrspace(3)* %[[C2]], i64 0
  ; CHECK-OPAQUE: %[[GEP:.*]] = getelementptr inbounds i32, ptr addrspace(3) %[[C2]], i64 0
  %arrayidx = getelementptr inbounds i32, i32 addrspace(4)* %call1, i64 0
  ; CHECK-TYPED: store i32 5, i32 addrspace(3)* %[[GEP]], align 4
  ; CHECK-OPAQUE: store i32 5, ptr addrspace(3) %[[GEP]], align 4
  store i32 5, i32 addrspace(4)* %arrayidx, align 4
  ret void
}

declare spir_func <3 x i64> @__builtin_spirv_BuiltInGlobalInvocationId()

!igc.functions = !{!0, !3, !4, !5}

!0 = !{void ()* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{i32 addrspace(4)* (i32)* @foo, !1}
!4 = !{i32 addrspace(4)* ()* @f0, !1}
!5 = !{i32 addrspace(4)* ()* @f1, !1}

; CHECK-TYPED: !{i32 addrspace(3)* ()* @f0, !1}
; CHECK-OPAQUE: !{ptr @f0, !1}
; CHECK-TYPED: !{i32 addrspace(3)* ()* @f1, !1}
; CHECK-OPAQUE: !{ptr @f1, !1}
; CHECK-TYPED: !{i32 addrspace(3)* (i32)* @foo, !1}
; CHECK-OPAQUE: !{ptr @foo, !1}
