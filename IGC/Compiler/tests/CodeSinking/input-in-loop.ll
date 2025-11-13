;
; Copyright (C) 2025 Intel Corporation
;
; This software and the related documents are Intel copyrighted materials,
; and your use of them is governed by the express license under which they were
; provided to you ("License"). Unless the License provides otherwise,
; you may not use, modify, copy, publish, distribute, disclose or transmit this
; software or the related documents without Intel's prior written permission.
;
; This software and the related documents are provided as is, with no express or
; implied warranties, other than those that are expressly stated in the License.
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers --igc-code-sinking --regkey CodeSinkingMinSize=1 -S %s | FileCheck %s

;CHECK-LABEL: entry:
;CHECK-NOT: @llvm.genx.GenISA.DCL.inputVec.f32
;CHECK-LABEL: loop:
;CHECK-NEXT:  [[IN1:%.*]] = phi float [ 0.000000e+00, %preheader ], [ [[IN0:%.*]], %loop ]
;CHECK: [[IN0]] = call float @llvm.genx.GenISA.DCL.inputVec.f32(i32 0, i32 2)
;CHECK: br i1 [[CMP:%.*]], label %loop, label %afterloop

define float @foo(i32 %count) {

entry:
  %input = call float @llvm.genx.GenISA.DCL.inputVec.f32(i32 0, i32 2)
  br label %preheader

preheader:
   br label %loop

loop:
  %input1 = phi float [ 0.000000e+00, %preheader ], [ %input, %loop ]
  %index = phi i32 [ 0, %preheader ], [ %inc, %loop ]
  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:
  %res = fadd float %input1, %input
  ret float %res
}

declare float @llvm.genx.GenISA.DCL.inputVec.f32(i32, i32)  #1

attributes #1 = { nounwind readnone willreturn }

!igc.functions = !{}

