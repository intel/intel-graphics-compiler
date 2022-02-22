;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -igc-type-legalizer | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

%"vec3xi4" = type { <3 x i4> }

; CHECK-LABEL: define void @testExtractElement
define void @testExtractElement(i16) {
  %a = trunc i16 %0 to i12
  ; CHECK: %[[AND1:.*]] = and i16 %0, 4095
  %b = bitcast i12 %a to <3 x i4>
  %e0 = extractelement <3 x i4> %b, i32 0
  ; CHECK: %[[AND2:.*]] = and i16 %[[AND1]], 15
  ; CHECK: %[[SHL:.*]] = shl i16 %[[AND2]], 4
  ; CHECK: %[[TR:.*]] = trunc i16 %[[SHL]] to i8
  ; CHECK: %[[E0:.*]] = ashr i8 %[[TR]], 4
  %e1 = extractelement <3 x i4> %b, i32 1
  ; CHECK: %[[AND:.*]] = and i16 %[[AND1]], 240
  ; CHECK: %[[TR:.*]] = trunc i16 %[[AND]] to i8
  ; CHECK: %[[E1:.*]] = ashr i8 %[[TR]], 4
  %e2 = extractelement <3 x i4> %b, i32 2
  ; CHECK: %[[AND:.*]] = and i16 %[[AND1]], 3840
  ; CHECK: %[[SHR:.*]] = lshr i16 %[[AND]], 4
  ; CHECK: %[[TR:.*]] = trunc i16 %[[SHR]] to i8
  ; CHECK: %[[E2:.*]] = ashr i8 %[[TR]], 4
  ret void
}

; CHECK-LABEL: define void @testInsertElement
define void @testInsertElement(i8) {
  %a = trunc i8 %0 to i4
  ; CHECK: %[[AND3:.*]] = and i8 %0, 15
  %v0 = insertelement <3 x i4> undef, i4 %a, i32 0
  ; CHECK: %[[V0:.*]] = zext i8 %[[AND3]] to i16
  %v1 = insertelement <3 x i4> %v0, i4 %a, i32 1
  ; CHECK: %[[ZEXT:.*]] = zext i8 %[[AND3]] to i16
  ; CHECK: %[[SHL:.*]] = shl i16 %[[ZEXT]], 4
  ; CHECK: %[[V1:.*]] = and i16 %[[V0]], %[[SHL]]
  %v2 = insertelement <3 x i4> %v1, i4 %a, i32 2
  ; CHECK: %[[ZEXT:.*]] = zext i8 %[[AND3]] to i16
  ; CHECK: %[[SHL:.*]] = shl i16 %[[ZEXT]], 8
  ; CHECK: %[[V2:.*]] = and i16 %[[V1]], %[[SHL]]
  ret void
}


