;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S --inputcs --platformdg2 -o - -igc-constant-coalescing -dce | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

; Test that the load is unmodified, load size matches supported LSC size
define i32 @f0() {
entry:
  %base_addr = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
  %adr = add i64 %base_addr, 12
  %ptr = inttoptr i64 %adr to <3 x i32> addrspace(2)*
  %data = load <3 x i32>, <3 x i32> addrspace(2)* %ptr, align 16
  %a = extractelement <3 x i32> %data, i32 0
  %b = extractelement <3 x i32> %data, i32 1
  %c = extractelement <3 x i32> %data, i32 2
  %ab = add i32 %a, %b
  %abc = add i32 %ab, %c
  ret i32 %abc
}
 ; CHECK-LABEL: define i32 @f0
 ; CHECK: [[PTR:%.*]] = inttoptr i64 %adr to <3 x i32> addrspace(2)*
 ; CHECK: [[DATA:%.*]] = load <3 x i32>, <3 x i32> addrspace(2)* [[PTR]], align 16
 ; CHECK: [[A:%.*]] = extractelement <3 x i32> [[DATA]], i32 0
 ; CHECK: [[B:%.*]] = extractelement <3 x i32> [[DATA]], i32 1
 ; CHECK: [[C:%.*]] = extractelement <3 x i32> [[DATA]], i32 2
 ; CHECK: [[AB:%.*]] = add i32 [[A]], [[B]]
 ; CHECK: [[ABC:%.*]] = add i32 [[AB]], [[C]]
 ; CHECK: ret i32 [[ABC]]

; Test that the load size is reduced to the smallest size supported by LSC
define i32 @f1() {
entry:
  %base_addr = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
  %adr = add i64 %base_addr, 16
  %ptr = inttoptr i64 %adr to <8 x i32> addrspace(2)*
  %data = load <8 x i32>, <8 x i32> addrspace(2)* %ptr, align 8
  %a = extractelement <8 x i32> %data, i32 0
  %b = extractelement <8 x i32> %data, i32 1
  %c = extractelement <8 x i32> %data, i32 2
  %ab = add i32 %a, %b
  %abc = add i32 %ab, %c
  ret i32 %abc
}
 ; CHECK-LABEL: define i32 @f1
 ; CHECK: [[ADDR:%.*]] = add i64 %base_addr, 16
 ; CHECK: [[PTR:%.*]] = inttoptr i64 [[ADDR]] to <3 x i32> addrspace(2)*
 ; CHECK: [[DATA:%.*]] = load <3 x i32>, <3 x i32> addrspace(2)* [[PTR]], align 8
 ; CHECK: [[A:%.*]] = extractelement <3 x i32> [[DATA]], i32 0
 ; CHECK: [[B:%.*]] = extractelement <3 x i32> [[DATA]], i32 1
 ; CHECK: [[C:%.*]] = extractelement <3 x i32> [[DATA]], i32 2
 ; CHECK: [[AB:%.*]] = add i32 [[A]], [[B]]
 ; CHECK: [[ABC:%.*]] = add i32 [[AB]], [[C]]
 ; CHECK: ret i32 [[ABC]]



; Function Attrs: nounwind readnone
declare i64 @llvm.genx.GenISA.RuntimeValue.i64(i32) #0

attributes #0 = { nounwind readnone }

!igc.functions = !{!3, !4}

!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{i32 ()* @f0, !1}
!4 = !{i32 ()* @f1, !1}
