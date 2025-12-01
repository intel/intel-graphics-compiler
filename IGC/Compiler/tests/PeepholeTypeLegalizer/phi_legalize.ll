;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-int-type-legalizer -S < %s | FileCheck %s

; Test checks phi types legalization

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32-p2490368:32:32:32"

define void @test_2xi256(i1 %flag) {
; CHECK-LABEL: define void @test_2xi256
; CHECK: = phi <16 x i32> [ zeroinitializer, %bb1 ], [ zeroinitializer, %bb2 ]
bb:
  br i1 %flag, label %bb1, label %bb2

bb1:                                              ; preds = %bb
  br label %bb3

bb2:                                              ; preds = %bb
  br label %bb3

bb3:                                              ; preds = %bb2, %bb1
  %tmp = phi <2 x i256> [ zeroinitializer, %bb1 ], [ zeroinitializer, %bb2 ]
  br label %bb4

bb4:                                              ; preds = %bb3
  ret void
}

define void @test_2xi80(i1 %flag) {
; CHECK-LABEL: define void @test_2xi80
; CHECK: = phi <5 x i32> [ zeroinitializer, %bb1 ], [ zeroinitializer, %bb2 ]
bb:
  br i1 %flag, label %bb1, label %bb2

bb1:                                              ; preds = %bb
  br label %bb3

bb2:                                              ; preds = %bb
  br label %bb3

bb3:                                              ; preds = %bb2, %bb1
  %tmp = phi <2 x i80> [ zeroinitializer, %bb1 ], [ zeroinitializer, %bb2 ]
  br label %bb4

bb4:                                              ; preds = %bb3
  ret void
}

!igc.functions = !{}
