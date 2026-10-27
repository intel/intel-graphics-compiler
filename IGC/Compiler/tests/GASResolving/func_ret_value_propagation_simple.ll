;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - -igc-gas-ret-value-propagator | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - -igc-gas-ret-value-propagator | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define internal spir_func i32 addrspace(3)* @f0() {
  %address = alloca i64, align 8
  store i64 305419896, i64* %address, align 8
  %1 = load i64, i64* %address, align 8
  %2 = inttoptr i64 %1 to i32 addrspace(3)*
  ret i32 addrspace(3)* %2
}

; CHECK-TYPED: define internal spir_func i32 addrspace(3)* @f1() {
; CHECK-OPAQUE: define internal spir_func ptr addrspace(3) @f1() {
; CHECK-TYPED: %[[CALL_F0:.*]] = call spir_func i32 addrspace(3)* @f0()
; CHECK-OPAQUE: %[[CALL_F0:.*]] = call spir_func ptr addrspace(3) @f0()
; CHECK-TYPED-NOT: addrspacecast i32 addrspace(3)* %[[CALL_F0]] to i32 addrspace(4)*
; CHECK-OPAQUE-NOT: addrspacecast ptr addrspace(3) %[[CALL_F0]] to ptr addrspace(4)
; CHECK-TYPED: ret i32 addrspace(3)* %[[CALL_F0]]
; CHECK-OPAQUE: ret ptr addrspace(3) %[[CALL_F0]]
define internal spir_func i32 addrspace(4)* @f1() {
  %call = call spir_func i32 addrspace(3)* @f0()
  %call.ascast = addrspacecast i32 addrspace(3)* %call to i32 addrspace(4)*
  ret i32 addrspace(4)* %call.ascast
}

; CHECK-TYPED: define internal spir_func i32 addrspace(3)* @f2() {
; CHECK-OPAQUE: define internal spir_func ptr addrspace(3) @f2() {
; CHECK-TYPED: %[[CALL_F1:.*]] = call spir_func i32 addrspace(3)* @f1()
; CHECK-OPAQUE: %[[CALL_F1:.*]] = call spir_func ptr addrspace(3) @f1()
; CHECK-TYPED: ret i32 addrspace(3)* %[[CALL_F1]]
; CHECK-OPAQUE: ret ptr addrspace(3) %[[CALL_F1]]
define internal spir_func i32 addrspace(4)* @f2() {
  %call = call spir_func i32 addrspace(4)* @f1()
  ret i32 addrspace(4)* %call
}

; CHECK: define spir_kernel void @kernel() {
; CHECK-TYPED: %[[CALL_F2:.*]] = call spir_func i32 addrspace(3)* @f2()
; CHECK-OPAQUE: %[[CALL_F2:.*]] = call spir_func ptr addrspace(3) @f2()
; CHECK-TYPED: %[[GEP:.*]] = getelementptr inbounds i32, i32 addrspace(3)* %[[CALL_F2]], i64 0
; CHECK-OPAQUE: %[[GEP:.*]] = getelementptr inbounds i32, ptr addrspace(3) %[[CALL_F2]], i64 0
; CHECK-TYPED: store i32 5, i32 addrspace(3)* %[[GEP]], align 4
; CHECK-OPAQUE: store i32 5, ptr addrspace(3) %[[GEP]], align 4
define spir_kernel void @kernel() {
  %call = call spir_func i32 addrspace(4)* @f2()
  %arrayidx = getelementptr inbounds i32, i32 addrspace(4)* %call, i64 0
  store i32 5, i32 addrspace(4)* %arrayidx, align 4
  ret void
}

!igc.functions = !{!0, !3, !4, !5}

!0 = !{void ()* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{i32 addrspace(3)* ()* @f0, !1}
; CHECK-TYPED: !{i32 addrspace(3)* ()* @f1, !1}
; CHECK-OPAQUE: !{ptr @f1, !1}
!4 = !{i32 addrspace(4)* ()* @f1, !1}
; CHECK-TYPED: !{i32 addrspace(3)* ()* @f2, !1}
; CHECK-OPAQUE: !{ptr @f2, !1}
!5 = !{i32 addrspace(4)* ()* @f2, !1}

