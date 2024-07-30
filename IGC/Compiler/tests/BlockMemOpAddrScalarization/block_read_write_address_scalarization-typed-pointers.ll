;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -block-memop-addr-scalar -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)*, i32)
declare i32 @get.value()

define spir_kernel void @kernel1(i32 %src1, i32 %src2, i32 %src3, i32 %src4) {
entry:
  %id = call i32 @get.value()
  ; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32.i32.i32(i32 %id, i32 0, i32 0)
  br label %bb1
bb1:
  %add1 = add i32 %src1, %id
  ; CHECK:  %add1 = add i32 %src1, [[TMP1:%.*]]
  %add2 = add i32 %src2, %src3
  %mul1 = mul i32 %add1, %src2
  %mul2 = mul i32 %add2, %src1
  %div1 = add i32 %mul2, %mul1
  %inttp1 = inttoptr i32 %div1 to i32 addrspace(1)*
  call void @llvm.genx.GenISA.simdBlockWrite.p1i32.i32(i32 addrspace(1)* %inttp1, i32 %src4)
  br label %bb2
bb2:
  ret void
}

!igc.functions = !{!0}
!0 = !{void (i32, i32, i32, i32)* @kernel1, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
