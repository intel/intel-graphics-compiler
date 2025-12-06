;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; RUN: igc_opt -opaque-pointers -igc-divrem-increment-reduction -S < %s | FileCheck %s
; ------------------------------------------------
;
; out-of-order udiv/urem increment reduction, increments not in sequential order (x+3, x, x+1, x+2, x+4) and are indirect increments of the base for the fourth and fifth pair
; - Leave first udiv/urem pair unoptimized (x + 3)
; - Leave second udiv/urem pair unoptimized (x)
; - Optimize third udiv/urem pair (y = x + 1) based on second pair
; - Optimize fourth udiv/urem pair (z = y + 1) based on third pair
; - Optimize fifth udiv/urem pair (z + 2) based on first pair
; ------------------------------------------------

define void @test_out_of_order_udiv(i32 %a, i32 %b, ptr %dest1, ptr %dest2, ptr %dest3) {
; First DivRemPair, leave unoptimized, because no prior suitable candidate to perform increment reduction on (base = a, offset = 3)
; CHECK: %div3 = udiv i32 %add3, %b
; CHECK: %rem3 = urem i32 %add3, %b
  %add3 = add i32 %a, 3
  %div3 = udiv i32 %add3, %b
  %rem3 = urem i32 %add3, %b

; Second DivRemPair, leave unoptimized, because no prior suitable candidate to perform increment reduction on (base = a, offset = 0)
; CHECK: %div0 = udiv i32 %a, %b
; CHECK: %rem0 = urem i32 %a, %b
  %div0 = udiv i32 %a, %b
  %rem0 = urem i32 %a, %b

; Third DivRemPair, optimize based on second pair (base = a, offset = 1)
; CHECK: [[PREINC_REM:%.*]] = add i32 %rem0, 1
; CHECK-NEXT: [[PREINC_REM_TEST:%.*]] = icmp eq i32 [[PREINC_REM]], %b
; CHECK-NEXT: [[PREINC_QUO:%.*]] = add i32 %div0, 1
; CHECK-NEXT: [[NEW_NEXT_QUO:%.*]] = select i1 [[PREINC_REM_TEST]], i32 [[PREINC_QUO]], i32 %div0
; CHECK-NEXT: [[NEW_NEXT_REM:%.*]] = select i1 [[PREINC_REM_TEST]], i32 0, i32 [[PREINC_REM]]
; CHECK-NOT: udiv i32 %add1, %b
; CHECK-NOT: urem i32 %add1, %b
  %add1 = add i32 %a, 1
  %div1 = udiv i32 %add1, %b
  %rem1 = urem i32 %add1, %b

; Fourth DivRemPair, optimize based on third pair (base = add1, offset = 1 -> base = a, offset = 2)
; CHECK: [[PREINC_REM2:%.*]] = add i32 [[NEW_NEXT_REM]], 1
; CHECK-NEXT: [[PREINC_REM2_TEST:%.*]] = icmp eq i32 [[PREINC_REM2]], %b
; CHECK-NEXT: [[PREINC_QUO2:%.*]] = add i32 [[NEW_NEXT_QUO]], 1
; CHECK-NEXT: [[NEW_NEXT_QUO2:%.*]] = select i1 [[PREINC_REM2_TEST]], i32 [[PREINC_QUO2]], i32 [[NEW_NEXT_QUO]]
; CHECK-NEXT: [[NEW_NEXT_REM2:%.*]] = select i1 [[PREINC_REM2_TEST]], i32 0, i32 [[PREINC_REM2]]
; CHECK-NOT: udiv i32 %add2, %b
; CHECK-NOT: urem i32 %add2, %b
  %add2 = add i32 %add1, 1
  %div2 = udiv i32 %add2, %b
  %rem2 = urem i32 %add2, %b

; Fifth DivRemPair, optimize based on first pair (base = add2, offset = 2 -> base = a, offset = 4)
; CHECK: [[PREINC_REM4:%.*]] = add i32 %rem3, 1
; CHECK-NEXT: [[PREINC_REM4_TEST:%.*]] = icmp eq i32 [[PREINC_REM4]], %b
; CHECK-NEXT: [[PREINC_QUO4:%.*]] = add i32 %div3, 1
; CHECK-NEXT: [[NEW_NEXT_QUO4:%.*]] = select i1 [[PREINC_REM4_TEST]], i32 [[PREINC_QUO4]], i32 %div3
; CHECK-NEXT: [[NEW_NEXT_REM4:%.*]] = select i1 [[PREINC_REM4_TEST]], i32 0, i32 [[PREINC_REM4]]
; CHECK-NOT: udiv i32 %add4, %b
; CHECK-NOT: urem i32 %add4, %b
  %add4 = add i32 %add2, 2
  %div4 = udiv i32 %add4, %b
  %rem4 = urem i32 %add4, %b

  %sum_div = add i32 %div3, %div0
; CHECK: [[SUM_DIV2:%.*]] = add i32 [[NEW_NEXT_QUO]], [[NEW_NEXT_QUO2]]
  %sum_div2 = add i32 %div1, %div2
  %res_div = add i32 %sum_div, %sum_div2
  %sum_rem = add i32 %rem3, %rem0
; CHECK: [[SUM_REM2:%.*]] = add i32 [[NEW_NEXT_REM]], [[NEW_NEXT_REM2]]
  %sum_rem2 = add i32 %rem1, %rem2
  %res_rem = add i32 %sum_rem, %sum_rem2
  store i32 %res_div, ptr %dest1
  store i32 %res_rem, ptr %dest2
; CHECK: %res4 = add i32 [[NEW_NEXT_QUO4]], [[NEW_NEXT_REM4]]
  %res4 = add i32 %div4, %rem4
  store i32 %res4, ptr %dest3
  ret void
}
