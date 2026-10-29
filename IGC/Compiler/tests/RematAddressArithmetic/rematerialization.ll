;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - -igc-remat-address-arithmetic | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - -igc-remat-address-arithmetic | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}

; Function pattern:
;
; kernel void foo(int cond) {
;     int x = 0;
;     if(cond>0)
;     {
;         x = 1;
;     }
;     else if(cond<0)
;     {
;         x = 2;
;     }
;     else
;     {
;         x = 3;
;     }
; }

define spir_kernel void @foo(i8* %privateBase, i32 %cond) {
; CHECK-TYPED: [[BASE:%.*]] = ptrtoint i8* %privateBase to i32
; CHECK-OPAQUE: [[BASE:%.*]] = ptrtoint ptr %privateBase to i32
  %base = ptrtoint i8* %privateBase to i32
; CHECK: [[ADDRESS:%.*]] = add i32 {{%.*}}, 8
  %address = add i32 %base, 8
; CHECK-TYPED: [[PTR:%.*]] = inttoptr i32 [[ADDRESS]] to i32*
; CHECK-OPAQUE: [[PTR:%.*]] = inttoptr i32 [[ADDRESS]] to ptr{{$}}
  %ptr = inttoptr i32 %address to i32*
; CHECK-TYPED: store i32 0, i32* [[PTR]], align 4
; CHECK-OPAQUE: store i32 0, ptr [[PTR]], align 4
  store i32 0, i32* %ptr, align 4
  %compare = icmp sgt i32 %cond, 0
  br i1 %compare, label %if, label %elseif

if:
; CHECK: [[RECALC_ADDR:%.*]] = add i32 [[BASE]], 8
; CHECK-TYPED: [[REMAT_ADDRESS:%.*]] = inttoptr i32 [[RECALC_ADDR]] to i32*
; CHECK-OPAQUE: [[REMAT_ADDRESS:%.*]] = inttoptr i32 [[RECALC_ADDR]] to ptr{{$}}
  %ptr1 = inttoptr i32 %address to i32*
; CHECK-TYPED: store i32 1, i32* [[REMAT_ADDRESS]], align 4
; CHECK-OPAQUE: store i32 1, ptr [[REMAT_ADDRESS]], align 4
  store i32 1, i32* %ptr1, align 4
  br label %exit

elseif:
  %compare2 = icmp slt i32 %cond, 0
  br i1 %compare2, label %elseif2, label %else

elseif2:
; CHECK: [[RECALC_ADDR:%.*]] = add i32 [[BASE]], 8
; CHECK-TYPED: [[REMAT_ADDRESS:%.*]] = inttoptr i32 [[RECALC_ADDR]] to i32*
; CHECK-OPAQUE: [[REMAT_ADDRESS:%.*]] = inttoptr i32 [[RECALC_ADDR]] to ptr{{$}}
  %ptr2 = inttoptr i32 %address to i32*
; CHECK-TYPED: store i32 2, i32* [[REMAT_ADDRESS]], align 4
; CHECK-OPAQUE: store i32 2, ptr [[REMAT_ADDRESS]], align 4
  store i32 2, i32* %ptr2, align 4
  br label %exit

else:
; CHECK: [[RECALC_ADDR:%.*]] = add i32 [[BASE]], 8
; CHECK-TYPED: [[REMAT_ADDRESS:%.*]] = inttoptr i32 [[RECALC_ADDR]] to i32*
; CHECK-OPAQUE: [[REMAT_ADDRESS:%.*]] = inttoptr i32 [[RECALC_ADDR]] to ptr{{$}}
  %ptr3 = inttoptr i32 %address to i32*
; CHECK-TYPED: store i32 3, i32* [[REMAT_ADDRESS]], align 4
; CHECK-OPAQUE: store i32 3, ptr [[REMAT_ADDRESS]], align 4
  store i32 3, i32* %ptr3, align 4
  br label %exit

exit:
  ret void
}
