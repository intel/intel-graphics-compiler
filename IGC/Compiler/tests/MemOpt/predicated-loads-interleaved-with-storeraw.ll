;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that two SLM predicated loads interleaved with a GenISA.storeraw are correctly merged.

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o -  -igc-address-space-alias-analysis -igc-aa-wrapper -igc-memopt | FileCheck %s

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a0:0:32-n8:16:32-S32"

define void @f0(i32 %addr, i32 %bso, i32 %offset) {
entry:
  %addr0 = add i32 %addr, 16
  %ptr0 = inttoptr i32 %addr0 to half addrspace(3)*
  %data0 = call half @llvm.genx.GenISA.PredicatedLoad.f16.p3f16.f16(half addrspace(3)* %ptr0, i64 2, i1 true, half 0xH5200)
  %bufHandle = inttoptr i32 %bso to i8 addrspace(2490371)*
  %offset0 = add i32 %offset, 16
  call void @llvm.genx.GenISA.storeraw.indexed.p2490371i8.f16(i8 addrspace(2490371)* %bufHandle, i32 %offset0, half %data0, i32 2, i1 false)
  %addr1 = add i32 %addr, 18
  %ptr1 = inttoptr i32 %addr1 to half addrspace(3)*
  %data1 = call half @llvm.genx.GenISA.PredicatedLoad.f16.p3f16.f16(half addrspace(3)* %ptr1, i64 2, i1 true, half 0xH5201)
  %offset1 = add i32 %offset, 18
  call void @llvm.genx.GenISA.storeraw.indexed.p2490371i8.f16(i8 addrspace(2490371)* %bufHandle, i32 %offset1, half %data1, i32 2, i1 false)
  ret void
}
; CHECK-LABEL: define void @f0
; CHECK: %addr0 = add i32 %addr, 16
; CHECK: %ptr0 = inttoptr i32 %addr0 to half addrspace(3)*
; CHECK: [[PTR:%.*]] = bitcast half addrspace(3)* %ptr0 to <2 x half> addrspace(3)*
; CHECK: [[V2HALF:%.*]] = call <2 x half> @llvm.genx.GenISA.PredicatedLoad.v2f16.p3v2f16.v2f16(<2 x half> addrspace(3)* [[PTR]], i64 2, i1 true, <2 x half> <half 0xH5200, half 0xH5201>)
; CHECK: [[DATA0:%.*]] = extractelement <2 x half> [[V2HALF]], i32 0
; CHECK: [[DATA1:%.*]] = extractelement <2 x half> [[V2HALF]], i32 1
; CHECK: %bufHandle = inttoptr i32 %bso to i8 addrspace(2490371)*
; CHECK: %offset0 = add i32 %offset, 16
; CHECK: call void @llvm.genx.GenISA.storeraw.indexed.p2490371i8.f16(i8 addrspace(2490371)* %bufHandle, i32 %offset0, half [[DATA0]], i32 2, i1 false)
; CHECK: %offset1 = add i32 %offset, 18
; CHECK: call void @llvm.genx.GenISA.storeraw.indexed.p2490371i8.f16(i8 addrspace(2490371)* %bufHandle, i32 %offset1, half [[DATA1]], i32 2, i1 false)
; CHECK: ret void

declare void @llvm.genx.GenISA.storeraw.indexed.p2490371i8.f16(i8 addrspace(2490371)*, i32, half, i32, i1) #7

; Function Attrs: nounwind readonly
declare half @llvm.genx.GenISA.PredicatedLoad.f16.p3f16.f16(half addrspace(3)*, i64, i1, half) #0

attributes #0 = { nounwind readonly }

!igc.functions = !{!0}

!0 = !{void (i32, i32, i32)* @f0, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
