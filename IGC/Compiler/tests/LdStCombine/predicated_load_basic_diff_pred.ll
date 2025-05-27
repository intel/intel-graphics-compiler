;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
;
; RUN:   igc_opt --typed-pointers %s -S -inputocl -igc-ldstcombine -regkey=EnableLdStCombine=5 \
; RUN:           -platformbmg \
; RUN: | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_basic(i32 addrspace(1)* %d, i32 addrspace(1)* %si, i16 %localIdX)
{
entry:
  %conv.i.i = zext i16 %localIdX to i64
;
; no merging due to predicates are different
;
; CHECK-LABEL: define spir_kernel void @test_basic
; CHECK: %c0.0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c0.arrayidx, i64 4, i1 true, i8 5)
; CHECK: %c0.1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c0.arrayidx.1, i64 1, i1 false, i8 6)
; CHECK-NOT: <2 x i8> @llvm.genx.GenISA.PredicatedLoad
;
  %c0.si = bitcast i32 addrspace(1)* %si to i8 addrspace(1)*
  %c0.arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %c0.si, i64 %conv.i.i
  %c0.0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c0.arrayidx, i64 4, i1 true, i8 5)
  %c0.add.1 = add nuw nsw i64 %conv.i.i, 1
  %c0.arrayidx.1 = getelementptr inbounds i8, i8 addrspace(1)* %c0.si, i64 %c0.add.1
  %c0.1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)* %c0.arrayidx.1, i64 1, i1 false, i8 6)
  %c0.0.0 = zext i8 %c0.0 to i16
  %c0.1.0 = zext i8 %c0.1 to i16
  %c0.1.1 = shl i16 %c0.1.0, 8
  %c0.e0  = add i16 %c0.1.1, %c0.0.0
  %c0.d = bitcast i32 addrspace(1)* %d to i16 addrspace(1)*
  %c0.idx = add nuw nsw i64 %conv.i.i, 11
  %c0.arrayidx1 = getelementptr inbounds i16, i16 addrspace(1)* %c0.d, i64 %c0.idx
  store i16 %c0.e0, i16 addrspace(1)* %c0.arrayidx1, align 4

; CHECK-LABEL: ret void
  ret void
}

; Function Attrs: nounwind readonly
declare i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1i8.i8(i8 addrspace(1)*, i64, i1, i8) #0

attributes #0 = { nounwind readonly }
