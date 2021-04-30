;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt -igc-legalization -verify -S %s -o - | FileCheck %s
; XFAIL: *
; Skip until the CodegenContext::shaderEntry issue is resolved

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_64_GEN9"

define i32 @A(float addrspace(13)* %P) #0 {
; CHECK-LABEL: A
; CHECK: %I = ptrtoint float addrspace(13)* %P to i32
; CHECK: ret i32 %I
  %I = ptrtoint float addrspace(13)* %P to i32
  ret i32 %I
}

define i32 @B(float addrspace(3)* %P) #0 {
; CHECK-LABEL: B
; CHECK: %1 = ptrtoint float addrspace(3)* %P to i64
; CHECK: %2 = trunc i64 %1 to i32
; CHECK: ret i32 %2
  %I = ptrtoint float addrspace(3)* %P to i32
  ret i32 %I
}

define float addrspace(3)* @C(i32 %I) #0 {
; CHECK-LABEL: C
; CHECK: %1 = zext i32 %I to i64
; CHECK: %P = inttoptr i64 %1 to float addrspace(3)*
; CHECK: ret float addrspace(3)* %P
  %P = inttoptr i32 %I to float addrspace(3)*
  ret float addrspace(3)* %P
}

define float addrspace(13)* @D(i32 %I) #0 {
; CHECK-LABEL: D
; CHECK: %P = inttoptr i32 %I to float addrspace(13)*
; CHECK: ret float addrspace(13)* %P
  %P = inttoptr i32 %I to float addrspace(13)*
  ret float addrspace(13)* %P
}

define i64 @E(float addrspace(13)* %P) #0 {
; CHECK-LABEL: E
; CHECK: %1 = ptrtoint float addrspace(13)* %P to i32
; CHECK: %2 = zext i32 %1 to i64
; CHECK: ret i64 %2
  %I = ptrtoint float addrspace(13)* %P to i64
  ret i64 %I
}

define i64 @F(float addrspace(3)* %P) #0 {
; CHECK-LABEL: F
; CHECK: %I = ptrtoint float addrspace(3)* %P to i64
; CHECK: ret i64 %I
  %I = ptrtoint float addrspace(3)* %P to i64
  ret i64 %I
}

define float addrspace(3)* @G(i64 %I) #0 {
; CHECK-LABEL: G
; CHECK: %P = inttoptr i64 %I to float addrspace(3)*
; CHECK: ret float addrspace(3)* %P
  %P = inttoptr i64 %I to float addrspace(3)*
  ret float addrspace(3)* %P
}

define float addrspace(13)* @H(i64 %I) #0 {
; CHECK-LABEL: H
; CHECK: %1 = trunc i64 %I to i32
; CHECK: %P = inttoptr i32 %1 to float addrspace(13)*
; CHECK: ret float addrspace(13)* %P
  %P = inttoptr i64 %I to float addrspace(13)*
  ret float addrspace(13)* %P
}

attributes #0 = { alwaysinline nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-frame-pointer-elim-non-leaf"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "unsafe-fp-math"="false" "use-soft-float"="false" }
