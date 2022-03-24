;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -igc-gas-resolve | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define internal spir_func void @f0(i32 addrspace(3)* %local_ptr) {
  %generic_alloca = alloca i32 addrspace(4)*, align 8
  %1 = addrspacecast i32 addrspace(3)* %local_ptr to i32 addrspace(4)*
  store i32 addrspace(4)* %1, i32 addrspace(4)** %generic_alloca, align 8
  %2 = load i32 addrspace(4)*, i32 addrspace(4)** %generic_alloca, align 8
  ; CHECK: addrspacecast i32 addrspace(4)* %2 to i32 addrspace(3)*
  ; CHECK: store i32 5, i32 addrspace(3)* %3, align 8
  store i32 5, i32 addrspace(4)* %2, align 8
  ret void
}

define internal spir_func void @f1(i32 %gid, i32 addrspace(3)* %local_ptr, i32 addrspace(1)* %global_ptr) {
  %generic_alloca = alloca i32 addrspace(4)*, align 8
  %rem = srem i32 %gid, 2
  %tobool = icmp ne i32 %rem, 0
  br i1 %tobool, label %if.then, label %if.else
if.then:
  %1 = addrspacecast i32 addrspace(3)* %local_ptr to i32 addrspace(4)*
  store i32 addrspace(4)* %1, i32 addrspace(4)** %generic_alloca, align 8
  br label %continue
if.else:
  %2 = addrspacecast i32 addrspace(1)* %global_ptr to i32 addrspace(4)*
  store i32 addrspace(4)* %2, i32 addrspace(4)** %generic_alloca, align 8
  br label %continue
continue:
  %3 = load i32 addrspace(4)*, i32 addrspace(4)** %generic_alloca, align 8
  ; Intentionally expecting that GASResolver won't resolve below store instruction, since it
  ; may operate on either global or local pointer (based on a control flow).
  ; CHECK-NOT: store i32 5, i32 addrspace(3)*
  ; CHECK-NOT: store i32 5, i32 addrspace(1)*
  ; CHECK: store i32 5, i32 addrspace(4)*
  store i32 5, i32 addrspace(4)* %3, align 8
  ret void
}

!igc.functions = !{!3, !4}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (i32 addrspace(3)*)* @f0, !1}
!4 = !{void (i32, i32 addrspace(3)*, i32 addrspace(1)*)* @f1, !1}

