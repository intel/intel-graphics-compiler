;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-lower-gp-arg | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"


; CHECK-LABEL: define void @f0
define void @f0(i32 addrspace(1)* %a0, i32 addrspace(1)* %a1) {
  ; CHECK-NOT: addrspacecast i32 addrspace(1)* %a0 to i32 addrspace(4)*
  ; CHECK-NOT: addrspacecast i32 addrspace(1)* %a1 to i32 addrspace(4)*
  %a0_generic = addrspacecast i32 addrspace(1)* %a0 to i32 addrspace(4)*
  %a1_generic = addrspacecast i32 addrspace(1)* %a1 to i32 addrspace(4)*

  ; CHECK: call void @f1(i32 addrspace(1)* %a0, i32 addrspace(1)* %a1)
  call void @f1(i32 addrspace(4)* %a0_generic, i32 addrspace(4)* %a1_generic)

  ret void
}

; CHECK: define void @f1(i32 addrspace(1)* %a0, i32 addrspace(1)* %a1)
define void @f1(i32 addrspace(4)* %a0, i32 addrspace(4)* %a1) {
  ; CHECK: call void @f2(i32 addrspace(1)* %a0, i32 addrspace(1)* %a1)
  call void @f2(i32 addrspace(4)* %a0, i32 addrspace(4)* %a1)

  ret void
}

; CHECK: define void @f2(i32 addrspace(1)* %a0, i32 addrspace(1)* %a1)
define void @f2(i32 addrspace(4)* %a0, i32 addrspace(4)* %a1) {
  ; CHECK: call void @f3(i32 addrspace(1)* %a0, i32 addrspace(1)* %a1)
  call void @f3(i32 addrspace(4)* %a0, i32 addrspace(4)* %a1)

  ret void
}

; CHECK: define void @f3(i32 addrspace(1)* %a0, i32 addrspace(1)* %a1)
define void @f3(i32 addrspace(4)* %a0, i32 addrspace(4)* %a1) {
  ; CHECK: store i32 0, i32 addrspace(1)* %a0, align 4
  store i32 0, i32 addrspace(4)* %a0, align 4
  ; CHECK: store i32 0, i32 addrspace(1)* %a1, align 4
  store i32 0, i32 addrspace(4)* %a1, align 4

  ret void
}

!igc.functions = !{!0, !3, !4, !5}

; CHECK: !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @f0, !1}
; CHECK: !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @f1, !1}
; CHECK: !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @f2, !1}
; CHECK: !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @f3, !1}
!0 = !{void (i32 addrspace(1)*, i32 addrspace(1)*)* @f0, !1}
!3 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @f1, !1}
!4 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @f2, !1}
!5 = !{void (i32 addrspace(4)*, i32 addrspace(4)*)* @f3, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
