;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-lower-gp-arg | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"


; CHECK-LABEL: define void @f0
define void @f0(ptr addrspace(1) %a0, ptr addrspace(1) %a1) {
  ; CHECK-NOT: addrspacecast ptr addrspace(1) %a0 to ptr addrspace(4)
  ; CHECK-NOT: addrspacecast ptr addrspace(1) %a1 to ptr addrspace(4)
  %a0_generic = addrspacecast ptr addrspace(1) %a0 to ptr addrspace(4)
  %a1_generic = addrspacecast ptr addrspace(1) %a1 to ptr addrspace(4)

  ; CHECK: call void @f1(ptr addrspace(1) %a0, ptr addrspace(1) %a1)
  call void @f1(ptr addrspace(4) %a0_generic, ptr addrspace(4) %a1_generic)

  ret void
}

; CHECK: define void @f1(ptr addrspace(1) %a0, ptr addrspace(1) %a1)
define void @f1(ptr addrspace(4) %a0, ptr addrspace(4) %a1) {
  ; CHECK: call void @f2(ptr addrspace(1) %a0, ptr addrspace(1) %a1)
  call void @f2(ptr addrspace(4) %a0, ptr addrspace(4) %a1)

  ret void
}

; CHECK: define void @f2(ptr addrspace(1) %a0, ptr addrspace(1) %a1)
define void @f2(ptr addrspace(4) %a0, ptr addrspace(4) %a1) {
  ; CHECK: call void @f3(ptr addrspace(1) %a0, ptr addrspace(1) %a1)
  call void @f3(ptr addrspace(4) %a0, ptr addrspace(4) %a1)

  ret void
}

; CHECK: define void @f3(ptr addrspace(1) %a0, ptr addrspace(1) %a1)
define void @f3(ptr addrspace(4) %a0, ptr addrspace(4) %a1) {
  ; CHECK: store i32 0, ptr addrspace(1) %a0, align 4
  store i32 0, ptr addrspace(4) %a0, align 4
  ; CHECK: store i32 0, ptr addrspace(1) %a1, align 4
  store i32 0, ptr addrspace(4) %a1, align 4

  ret void
}

!igc.functions = !{!0, !3, !4, !5}

; CHECK: !{ptr @f0, !1}
; CHECK: !{ptr @f1, !1}
; CHECK: !{ptr @f2, !1}
; CHECK: !{ptr @f3, !1}
!0 = !{ptr @f0, !1}
!3 = !{ptr @f1, !1}
!4 = !{ptr @f2, !1}
!5 = !{ptr @f3, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
