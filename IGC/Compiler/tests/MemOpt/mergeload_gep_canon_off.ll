;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S %enable-basic-aa% -platformpvc -igc-memopt --regkey=MemOptGEPCanon=1 %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_func i8 @sobel_grayscale(i8 addrspace(1)* %src, i32 %arg0, i32 %arg1, i32 %width) #0 {
entry:
  %sub20 = add nsw i32 %arg0, -1
  %sub21 = add nsw i32 %arg1, -1
  %mul22 = mul nsw i32 %sub21, %width
  %add23 = add nsw i32 %sub20, %mul22
  %idxprom24 = sext i32 %add23 to i64
  %arrayidx25 = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 %idxprom24
  %v0 = load i8, i8 addrspace(1)* %arrayidx25, align 1
  %add28 = add nsw i32 %arg0, %mul22
  %idxprom29 = sext i32 %add28 to i64
  %arrayidx30 = getelementptr inbounds i8, i8 addrspace(1)* %src, i64 %idxprom29
  %v1 = load i8, i8 addrspace(1)* %arrayidx30, align 1
  %sum = add i8 %v0, %v1
  ret i8 %sum
}

; Only one load after merging two i8 load!
;
; CHECK-LABEL: define spir_func i8 @sobel_grayscale
; CHECK:       load <2 x i8>
; CHECK-NOT:   load
; CHECK:       ret

define spir_func i32 @luxmark31_noise([512 x i32] addrspace(2)* %src, float %arg0, i32 %arg1) #1 {
entry:
  %conv = fptosi float %arg0 to i32
  %and = and i32 %conv, 255
  %add.i1 = add nsw i32 %arg1, %and
  %idxprom.i1 = zext i32 %add.i1 to i64
  %arrayidx.i1 = getelementptr inbounds [512 x i32], [512 x i32] addrspace(2)* %src, i64 0, i64 %idxprom.i1
  %val0 = load i32, i32 addrspace(2)* %arrayidx.i1, align 4
  %add.i2.1 = add nuw nsw i32 %and, 1
  %add.i2 = add nuw nsw i32 %arg1, %add.i2.1
  %idxprom.i2 = zext i32 %add.i2 to i64
  %arrayidx.i2 = getelementptr inbounds [512 x i32], [512 x i32] addrspace(2)* %src, i64 0, i64 %idxprom.i2
  %val1 = load i32, i32 addrspace(2)* %arrayidx.i2, align 4
  %total = add i32 %val0, %val1
  ret i32 %total
}

; CHECK-LABEL: define spir_func i32 @luxmark31_noise
; CHECK:       load <2 x i32>
; CHECK-NOT:   load
; CHECK:       ret


!igc.functions = !{!0, !3}

!0 = !{i8 (i8 addrspace(1)*, i32, i32, i32)* @sobel_grayscale, !1}
!3 = !{i32 ([512 x i32] addrspace(2)*, float, i32)* @luxmark31_noise, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
