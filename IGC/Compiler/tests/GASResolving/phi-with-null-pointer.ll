;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-gas-resolve | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define internal spir_func void @f0(i32 %gid) {
  %rem = srem i32 %gid, 2
  %tobool = icmp ne i32 %rem, 0
  br i1 %tobool, label %if.then, label %if.else
if.then:
  %1 = inttoptr i64 2271560481 to i8 addrspace(3)*
  ; CHECK-NOT: addrspacecast i8 addrspace(3)* %1 to i8 addrspace(4)*
  %2 = addrspacecast i8 addrspace(3)* %1 to i8 addrspace(4)*
  br label %continue
if.else:
  br label %continue
continue:
  ; CHECK: [[PHI:%.*]] = phi i8 addrspace(3)* [ null, %if.else ], [ %1, %if.then ]
  %3 = phi i8 addrspace(4)* [ null, %if.else ], [ %2, %if.then ]
  ; CHECK: store i8 5, i8 addrspace(3)* [[PHI]], align 4
  store i8 5, i8 addrspace(4)* %3, align 4
  ret void
}

