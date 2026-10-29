;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers %s -S -o - -igc-remat-address-arithmetic | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers %s -S -o - -igc-remat-address-arithmetic | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}

; This unit test only checks if pass is able to deal with infinite loops of uses in "phi" function
; Function pattern - infinite loop using %privateBase
;
; kernel void foo(int cond) {
;     int x = 0;
;         if(cond>0)
;             for(int i=0;;i++)
;                 x = i;
; }

define spir_kernel void @foo(i8* %privateBase, i32 %cond) {
; CHECK-TYPED: [[BASE:%.*]] = ptrtoint i8* %privateBase to i64
; CHECK-OPAQUE: [[BASE:%.*]] = ptrtoint ptr %privateBase to i64
  %base = ptrtoint i8* %privateBase to i64
; CHECK: [[OFFSET:%.*]] = add i64 [[BASE]], 8
  %offset = add i64 %base, 8
  %ptr1 = inttoptr i64 %offset to i32*
  store i32 1, i32* %ptr1, align 4
  %compare = icmp sgt i32 %cond, 0
  br i1 %compare, label %enter, label %exit

enter:
  br label %loop

loop:
  %indvar = phi i32 [ 0, %enter ], [ %nextindvar, %loop ]
  %nextindvar = add i32 %indvar, 1
; CHECK-TYPED: [[PTR:%.*]] = inttoptr i64 [[OFFSET]] to i32*
; CHECK-OPAQUE: [[PTR:%.*]] = inttoptr i64 [[OFFSET]] to ptr{{$}}
  %ptr2 = inttoptr i64 %offset to i32*
  store i32 %nextindvar, i32* %ptr2, align 4
  br label %loop

exit:
  ret void
}
