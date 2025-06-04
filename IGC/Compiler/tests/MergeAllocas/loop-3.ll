;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:32:32-v96:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

; RUN: igc_opt --opaque-pointers -S --platformbmg --igc-ocl-merge-allocas %s | FileCheck %s

; Test if lifetime leaking instruction will interact correctly with loops that contain the entirety of the lifetime
; CHECK-NOT: error

define void @test() #0 {
entry:
  %alloca1 = alloca i32
  br label %looppreheader

looppreheader:
  br label %loopheader

loopheader:
  %p2i = ptrtoint ptr %alloca1 to i64
  br i1 false, label %loopbody, label %if.end

loopbody:
  %a = load i32, ptr %alloca1
  store i32 10, ptr %alloca1
  br i1 false, label %exit, label %if.end

if.end:
  %b = load i32, ptr %alloca1
  br label %loopheader

exit:
  ret void
}

attributes #0 = { nounwind readnone }
