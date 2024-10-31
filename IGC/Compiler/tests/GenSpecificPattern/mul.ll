;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; RUN: igc_opt --opaque-pointers -debugify --igc-gen-specific-pattern -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern:
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_simple_mul_pow2(i32 %arg) {
  %1 = mul i32 %arg, 16
  call void @use.i32(i32 %1)
  ret void
}
; CHECK-LABEL: @test_simple_mul_pow2(i32 %arg
; CHECK:       %[[SHL:.+]] = shl i32 %arg, 4
; CHECK:       call void @use.i32(i32 %[[SHL]])
; CHECK:       ret void

; COM: Negative edge case test
define void @test_mul_pow2_skip_ones(i32 %arg) {
  %1 = mul i32 %arg, 1
  call void @use.i32(i32 %1)
  %2 = mul i32 -1, %arg
  call void @use.i32(i32 %2)
  ret void
}
; CHECK-LABEL: @test_mul_pow2_skip_ones(i32 %arg
; CHECK:       %[[MUL_PLUS:.+]] = mul i32 %arg, 1
; CHECK:       call void @use.i32(i32 %[[MUL_PLUS]])
; CHECK:       %[[MUL_MINUS:.+]] = mul i32 -1, %arg
; CHECK:       call void @use.i32(i32 %[[MUL_MINUS]])
; CHECK:       ret void

define void @test_mul_pow2_overflow_flags(i64 %arg) {
  %1 = mul nsw i64 %arg, 256
  call void @use.i64(i64 %1)
  %2 = mul nuw i64 %arg, 256
  call void @use.i64(i64 %2)
  %3 = mul nuw nsw i64 %arg, 1024
  call void @use.i64(i64 %3)
  ret void
}
; CHECK-LABEL: @test_mul_pow2_overflow_flags(i64 %arg
; CHECK:       %[[SHL_NSW:.+]] = shl nsw i64 %arg, 8
; CHECK:       call void @use.i64(i64 %[[SHL_NSW]])
; CHECK:       %[[SHL_NUW:.+]] = shl nuw i64 %arg, 8
; CHECK:       call void @use.i64(i64 %[[SHL_NUW]])
; CHECK:       %[[SHL_NUW_NSW:.+]] = shl nuw nsw i64 %arg, 10
; CHECK:       call void @use.i64(i64 %[[SHL_NUW_NSW]])
; CHECK:       ret void

define void @test_mul_neg_pow2_overflow_flags(i64 %arg) {
  %1 = mul nsw i64 %arg, -256
  call void @use.i64(i64 %1)
  %2 = mul nuw i64 %arg, -256
  call void @use.i64(i64 %2)
  %3 = mul nuw nsw i64 %arg, -1024
  call void @use.i64(i64 %3)
  ret void
}
; CHECK-LABEL: @test_mul_neg_pow2_overflow_flags(i64 %arg
; CHECK:       %[[SHL_NSW:.+]] = shl nsw i64 %arg, 8
; CHECK:       %[[NEG_NSW:.+]] = sub nsw i64 0, %[[SHL_NSW]]
; CHECK:       call void @use.i64(i64 %[[NEG_NSW]])
; CHECK:       %[[SHL_NUW:.+]] = shl nuw i64 %arg, 8
; CHECK:       %[[NEG_NUW:.+]] = sub nuw i64 0, %[[SHL_NUW]]
; CHECK:       call void @use.i64(i64 %[[NEG_NUW]])
; CHECK:       %[[SHL_NUW_NSW:.+]] = shl nuw nsw i64 %arg, 10
; CHECK:       %[[NEG_NUW_NSW:.+]] = sub nuw nsw i64 0, %[[SHL_NUW_NSW]]
; CHECK:       call void @use.i64(i64 %[[NEG_NUW_NSW]])
; CHECK:       ret void

define void @test_mul_neg_pow2_with_binops(i32 %arg0, i32 %arg1, i32 %arg2) {
entry:
; COM: The mul itself + simple use
  %0 = mul i32 %arg0, -1024
  call void @use.i32(i32 %0)
  br label %.add1
.add1:
; COM: mul + addend
  %1 = add i32 %0, %arg1
  call void @use.i32(i32 %1)
  br label %.add2
.add2:
; COM: addend + mul
  %2 = add i32 %arg2, %0
  call void @use.i32(i32 %2)
  ret void
}
; CHECK-LABEL: @test_mul_neg_pow2_with_binops(i32 %arg0, i32 %arg1, i32 %arg2
; COM: The mul itself + simple use
; CHECK:     entry:
; CHECK:       %[[SHL:.+]] = shl i32 %arg0, 10
; CHECK:       %[[NEG:.+]] = sub i32 0, %[[SHL]]
; CHECK:       call void @use.i32(i32 %[[NEG]])
; CHECK:     br label %.add1
; COM: mul + addend -> addend - shl
; CHECK:     .add1:
; CHECK:       %[[SUB1:.+]] = sub i32 %arg1, %[[SHL]]
; CHECK:       call void @use.i32(i32 %[[SUB1]])
; CHECK:       br label %.add2
; COM: addend + mul -> addend - shl
; CHECK:     .add2:
; CHECK:       %[[SUB2:.+]] = sub i32 %arg2, %[[SHL]]
; CHECK:       call void @use.i32(i32 %[[SUB2]])
; CHECK:       ret void

define void @test_mul_neg_pow2_with_binops_overflow_flags(i64 %arg0, i64 %arg1) {
; COM: 'mul' with no flags - 'add's with all combinations of flags
  %1 = mul i64 %arg0, -256
  %2 = add i64 %1, %arg1
  call void @use.i64(i64 %2)
  %3 = add nuw i64 %arg1, %1
  call void @use.i64(i64 %3)
  %4 = add nsw i64 %1, %arg1
  call void @use.i64(i64 %4)
  %5 = add nuw nsw i64 %arg1, %1
  call void @use.i64(i64 %5)
; COM: 'mul' with 'nuw' - 'add's with all combinations of flags
  %6 = mul nuw i64 %arg0, -1024
  %7 = add i64 %6, %arg1
  call void @use.i64(i64 %7)
  %8 = add nuw i64 %arg1, %6
  call void @use.i64(i64 %8)
  %9 = add nsw i64 %6, %arg1
  call void @use.i64(i64 %9)
  %10 = add nuw nsw i64 %arg1, %6
  call void @use.i64(i64 %10)
; COM: 'mul' with 'nsw' - 'add's with all combinations of flags
  %11 = mul nsw i64 %arg0, -512
  %12 = add i64 %11, %arg1
  call void @use.i64(i64 %12)
  %13 = add nuw i64 %arg1, %11
  call void @use.i64(i64 %13)
  %14 = add nsw i64 %11, %arg1
  call void @use.i64(i64 %14)
  %15 = add nuw nsw i64 %arg1, %11
  call void @use.i64(i64 %15)
; COM: 'mul' with 'nuw nsw' - 'add's with all combinations of flags
  %16 = mul nuw nsw i64 %arg0, -2048
  %17 = add i64 %16, %arg1
  call void @use.i64(i64 %17)
  %18 = add nuw i64 %arg1, %16
  call void @use.i64(i64 %18)
  %19 = add nsw i64 %16, %arg1
  call void @use.i64(i64 %19)
  %20 = add nuw nsw i64 %arg1, %16
  call void @use.i64(i64 %20)
  ret void
}
; CHECK-LABEL: @test_mul_neg_pow2_with_binops_overflow_flags(i64 %arg0, i64 %arg1
; COM: 'mul' with no flags
; CHECK:       %[[SHL_NO_FLAG:.+]] = shl i64 %arg0, 8
; CHECK:       %[[NEG_NO_FLAG:.+]] = sub i64 0, %[[SHL_NO_FLAG]]
; COM: 'add' with no flags is the only one to be simplified
; CHECK:       %[[ADD_NO_FLAG:.+]] = sub i64 %arg1, %[[SHL_NO_FLAG]]
; CHECK:       call void @use.i64(i64 %[[ADD_NO_FLAG]])
; CHECK:       %[[ADD_NUW:.+]] = add nuw i64 %arg1, %[[NEG_NO_FLAG]]
; CHECK:       call void @use.i64(i64 %[[ADD_NUW]])
; CHECK:       %[[ADD_NSW:.+]] = add nsw i64 %[[NEG_NO_FLAG]], %arg1
; CHECK:       call void @use.i64(i64 %[[ADD_NSW]])
; CHECK:       %[[ADD_NUW_NSW:.+]] = add nuw nsw i64 %arg1, %[[NEG_NO_FLAG]]
; CHECK:       call void @use.i64(i64 %[[ADD_NUW_NSW]])
; COM: 'mul' with 'nuw'
; CHECK:       %[[SHL_NUW:.+]] = shl nuw i64 %arg0, 10
; CHECK:       %[[NEG_NUW:.+]] = sub nuw i64 0, %[[SHL_NUW]]
; COM: 'add' with 'nuw' is the only one to be simplified
; CHECK:       %[[ADD_NO_FLAG:.+]] = add i64 %[[NEG_NUW]], %arg1
; CHECK:       call void @use.i64(i64 %[[ADD_NO_FLAG]])
; CHECK:       %[[ADD_NUW:.+]] = sub nuw i64 %arg1, %[[SHL_NUW]]
; CHECK:       call void @use.i64(i64 %[[ADD_NUW]])
; CHECK:       %[[ADD_NSW:.+]] = add nsw i64 %[[NEG_NUW]], %arg1
; CHECK:       call void @use.i64(i64 %[[ADD_NSW]])
; CHECK:       %[[ADD_NUW_NSW:.+]] = add nuw nsw i64 %arg1, %[[NEG_NUW]]
; CHECK:       call void @use.i64(i64 %[[ADD_NUW_NSW]])
; COM: 'mul' with 'nsw'
; CHECK:       %[[SHL_NSW:.+]] = shl nsw i64 %arg0, 9
; CHECK:       %[[NEG_NSW:.+]] = sub nsw i64 0, %[[SHL_NSW]]
; COM: 'add' with 'nsw' is the only one to be simplified
; CHECK:       %[[ADD_NO_FLAG:.+]] = add i64 %[[NEG_NSW]], %arg1
; CHECK:       call void @use.i64(i64 %[[ADD_NO_FLAG]])
; CHECK:       %[[ADD_NUW:.+]] = add nuw i64 %arg1, %[[NEG_NSW]]
; CHECK:       call void @use.i64(i64 %[[ADD_NUW]])
; CHECK:       %[[ADD_NSW:.+]] = sub nsw i64 %arg1, %[[SHL_NSW]]
; CHECK:       call void @use.i64(i64 %[[ADD_NSW]])
; CHECK:       %[[ADD_NUW_NSW:.+]] = add nuw nsw i64 %arg1, %[[NEG_NSW]]
; CHECK:       call void @use.i64(i64 %[[ADD_NUW_NSW]])
; COM: 'mul' with 'nuw nsw'
; CHECK:       %[[SHL_NUW_NSW:.+]] = shl nuw nsw i64 %arg0, 11
; CHECK:       %[[NEG_NUW_NSW:.+]] = sub nuw nsw i64 0, %[[SHL_NUW_NSW]]
; COM: 'add' with 'nuw nsw' is the only one to be simplified
; CHECK:       %[[ADD_NO_FLAG:.+]] = add i64 %[[NEG_NUW_NSW]], %arg1
; CHECK:       call void @use.i64(i64 %[[ADD_NO_FLAG]])
; CHECK:       %[[ADD_NUW:.+]] = add nuw i64 %arg1, %[[NEG_NUW_NSW]]
; CHECK:       call void @use.i64(i64 %[[ADD_NUW]])
; CHECK:       %[[ADD_NSW:.+]] = add nsw i64 %[[NEG_NUW_NSW]], %arg1
; CHECK:       call void @use.i64(i64 %[[ADD_NSW]])
; CHECK:       %[[ADD_NUW_NSW:.+]] = sub nuw nsw i64 %arg1, %[[SHL_NUW_NSW]]
; CHECK:       call void @use.i64(i64 %[[ADD_NUW_NSW]])
; CHECK:       ret void

declare void @use.i64(i64)
declare void @use.i32(i32)
