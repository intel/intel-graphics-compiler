;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -platformbmg -simd-mode 16 -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s

; %m coalesces with %s and %t, and %s is the root of that class. The moves into
; %m write the root's variable, so %n must read that variable when it copies %m.

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

@ThreadGroupSize_X = constant i32 16
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

; CHECK: .kernel "entry"
; CHECK: goto (M1, 16) _0_
; CHECK: goto (M1, 16) [[MERGE:_0_[0-9]+]]
; CHECK: mov (M1, 16) [[ROOT:V[0-9]+]](0,0)<1>
; CHECK: [[MERGE]]:
; CHECK: mov (M1, 16) {{V[0-9]+}}(0,0)<1> [[ROOT]](0,0)<1;1,0>

define void @entry(<8 x i32> %r0, i64 %inc, ptr %result) {
head:
  %lid16 = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %lid = zext i16 %lid16 to i32
  %state = zext i32 %lid to i64
  %s0 = insertvalue { i64, i64 } undef, i64 %state, 0
  %s = insertvalue { i64, i64 } %s0, i64 %inc, 1
  %flag = icmp ult i32 %lid, 8
  br i1 %flag, label %outer, label %join

outer:
  %flag2 = icmp ult i32 %lid, 4
  br i1 %flag2, label %inner, label %merge

inner:
  %state2 = mul i64 %state, 3
  %t0 = insertvalue { i64, i64 } undef, i64 %state2, 0
  %t = insertvalue { i64, i64 } %t0, i64 %inc, 1
  br label %merge

merge:
  %m = phi { i64, i64 } [ %t, %inner ], [ %s, %outer ]
  br label %join

join:
  %n = phi { i64, i64 } [ %m, %merge ], [ %s, %head ]
  %f = extractvalue { i64, i64 } %n, 0
  store i64 %f, ptr %result, align 8
  ret void
}

declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32)

!igc.functions = !{!1}
!1 = !{ptr @entry, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
