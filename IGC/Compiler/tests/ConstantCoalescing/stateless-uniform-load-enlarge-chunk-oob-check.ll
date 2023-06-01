;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S --inputcs --platformdg2 -o - -igc-constant-coalescing -dce | FileCheck %s

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

; Test that the chunk start is moved to not create OOB access and alignment is
; changed to 4
define i32 @f1() {
entry:
  %base_addr = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
  %adr = add i64 %base_addr, 24
  %ptr = inttoptr i64 %adr to <6 x i32> addrspace(2)*
  %data = load <6 x i32>, <6 x i32> addrspace(2)* %ptr, align 8
  %a = extractelement <6 x i32> %data, i32 0
  %b = extractelement <6 x i32> %data, i32 1
  %c = extractelement <6 x i32> %data, i32 2
  %d = extractelement <6 x i32> %data, i32 3
  %e = extractelement <6 x i32> %data, i32 4
  %f = extractelement <6 x i32> %data, i32 5
  %ab = add i32 %a, %b
  %abc = add i32 %ab, %c
  %abcd = add i32 %abc, %d
  %abcde = add i32 %abcd, %e
  %abcdef = add i32 %abcde, %f
  ret i32 %abcdef
}
 ; CHECK-LABEL: define i32 @f1
 ; CHECK: [[ADDR:%.*]] = add i64 %base_addr, 16
 ; CHECK: [[PTR:%.*]] = inttoptr i64 [[ADDR]] to <8 x i32> addrspace(2)*
 ; CHECK: [[DATA:%.*]] = load <8 x i32>, <8 x i32> addrspace(2)* [[PTR]], align 4
 ; CHECK: [[A:%.*]] = extractelement <8 x i32> [[DATA]], i32 2
 ; CHECK: [[B:%.*]] = extractelement <8 x i32> [[DATA]], i32 3
 ; CHECK: [[C:%.*]] = extractelement <8 x i32> [[DATA]], i32 4
 ; CHECK: [[D:%.*]] = extractelement <8 x i32> [[DATA]], i32 5
 ; CHECK: [[E:%.*]] = extractelement <8 x i32> [[DATA]], i32 6
 ; CHECK: [[F:%.*]] = extractelement <8 x i32> [[DATA]], i32 7
 ; CHECK: [[AB:%.*]] = add i32 [[A]], [[B]]
 ; CHECK: [[ABC:%.*]] = add i32 [[AB]], [[C]]
 ; CHECK: [[ABCD:%.*]] = add i32 [[ABC]], [[D]]
 ; CHECK: [[ABCDE:%.*]] = add i32 [[ABCD]], [[E]]
 ; CHECK: [[ABCDEF:%.*]] = add i32 [[ABCDE]], [[F]]
 ; CHECK: ret i32 [[ABCDEF]]


; Test that the chunk is enlarged due to the existence of the first load
define i32 @f2() {
entry:
  %base_addr = call i64 @llvm.genx.GenISA.RuntimeValue.i64(i32 0)
  %adr0 = add i64 %base_addr, 256
  %ptr0 = inttoptr i64 %adr0 to i32 addrspace(2)*
  %data0 = load i32, i32 addrspace(2)* %ptr0, align 4
  %adr = add i64 %base_addr, 24
  %ptr = inttoptr i64 %adr to <6 x i32> addrspace(2)*
  %data = load <6 x i32>, <6 x i32> addrspace(2)* %ptr, align 8
  %a = extractelement <6 x i32> %data, i32 0
  %b = extractelement <6 x i32> %data, i32 1
  %c = extractelement <6 x i32> %data, i32 2
  %d = extractelement <6 x i32> %data, i32 3
  %e = extractelement <6 x i32> %data, i32 4
  %f = extractelement <6 x i32> %data, i32 5
  %ab = add i32 %a, %b
  %abc = add i32 %ab, %c
  %abcd = add i32 %abc, %d
  %abcde = add i32 %abcd, %e
  %abcdef = add i32 %abcde, %f
  %abcdefg = add i32 %abcdef, %data0
  ret i32 %abcdefg
}
 ; CHECK-LABEL: define i32 @f2
 ; CHECK: [[ADDR0:%.*]] = add i64 %base_addr, 256
 ; CHECK: [[PTR0:%.*]] = inttoptr i64 [[ADDR0]] to <1 x i32> addrspace(2)*
 ; CHECK: [[DATA0:%.*]] = load <1 x i32>, <1 x i32> addrspace(2)* [[PTR0]], align 4
 ; CHECK: [[G:%.*]] = extractelement <1 x i32> [[DATA0]], i32 0
 ; CHECK: [[ADDR:%.*]] = add i64 %base_addr, 24
 ; CHECK: [[PTR:%.*]] = inttoptr i64 [[ADDR]] to <8 x i32> addrspace(2)*
 ; CHECK: [[DATA:%.*]] = load <8 x i32>, <8 x i32> addrspace(2)* [[PTR]], align 8
 ; CHECK: [[A:%.*]] = extractelement <8 x i32> [[DATA]], i32 0
 ; CHECK: [[B:%.*]] = extractelement <8 x i32> [[DATA]], i32 1
 ; CHECK: [[C:%.*]] = extractelement <8 x i32> [[DATA]], i32 2
 ; CHECK: [[D:%.*]] = extractelement <8 x i32> [[DATA]], i32 3
 ; CHECK: [[E:%.*]] = extractelement <8 x i32> [[DATA]], i32 4
 ; CHECK: [[F:%.*]] = extractelement <8 x i32> [[DATA]], i32 5
 ; CHECK: [[AB:%.*]] = add i32 [[A]], [[B]]
 ; CHECK: [[ABC:%.*]] = add i32 [[AB]], [[C]]
 ; CHECK: [[ABCD:%.*]] = add i32 [[ABC]], [[D]]
 ; CHECK: [[ABCDE:%.*]] = add i32 [[ABCD]], [[E]]
 ; CHECK: [[ABCDEF:%.*]] = add i32 [[ABCDE]], [[F]]
 ; CHECK: [[ABCDEFG:%.*]] = add i32 [[ABCDEF]], [[G]]
 ; CHECK: ret i32 [[ABCDEFG]]



; Function Attrs: nounwind readnone
declare i64 @llvm.genx.GenISA.RuntimeValue.i64(i32) #0

attributes #0 = { nounwind readnone }

!igc.functions = !{!3, !4, !5}

!1 = !{!2}
!2 = !{!"function_type", i32 0}

!3 = !{i32 ()* @f0, !1}
!4 = !{i32 ()* @f1, !1}
!5 = !{i32 ()* @f2, !1}
