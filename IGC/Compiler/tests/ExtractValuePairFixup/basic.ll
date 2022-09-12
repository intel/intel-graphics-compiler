;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-extractvalue-pair-fixup -S < %s | FileCheck %s
; ------------------------------------------------
; ExtractValuePairFixup
; ------------------------------------------------

; Case 1:
;   Nothing is moved
;
; CHECK: entry:
; CHECK: [[VAL1:%[A-z0-9]*]] = insertvalue %str {{.*}}, 0
; CHECK-NEXT: [[VAL2:%[A-z0-9]*]] = insertvalue %str [[VAL1]]{{.*}}, 1
; CHECK-NEXT: [[USE1:[%A-z0-9]*]] = extractvalue %str [[VAL2]], 1
; CHECK-NEXT: br label %lbl
;
; Case 2:
;
;   %5 = extractvalue %str %3, 1
;   moved after
;   %4 = extractvalue %str %0, 0
; CHECK: lbl:
; CHECK-NEXT: [[VAL3:%[A-z0-9]*]] = insertvalue %str [[VAL2]]{{.*}}, 1
; CHECK-NEXT: [[USE2:%[A-z0-9]*]] = extractvalue %str [[VAL1]], 0
; CHECK-NEXT: [[USE3:%[A-z0-9]*]] = extractvalue %str [[VAL3]], 1
; CHECK-NEXT: call void @use_i32(i32 [[USE2]])
; CHECK-NEXT: call void @use(i64 [[USE1]], i64 [[USE3]], %str [[VAL1]], %str [[VAL2]], %str [[VAL3]])

%str = type { i32, i64 }

define spir_kernel void @test_extr(i32 %src1, i64 %src2) {
entry:
  %0 = insertvalue %str { i32 0, i64 32 }, i32 %src1, 0
  %1 = insertvalue %str %0, i64 %src2, 1
  %2 = extractvalue %str %1, 1
  br label %lbl

lbl:
  %3 = insertvalue %str %1, i64 %2, 1
  %4 = extractvalue %str %0, 0
  call void @use_i32(i32 %4)
  %5 = extractvalue %str %3, 1
  call void @use(i64 %2, i64 %5, %str %0, %str %1, %str %3)
  ret void
}

declare void @use_i32(i32)
declare void @use(i64, i64, %str, %str, %str)

!igc.functions = !{!0}
!0 = !{void (i32, i64)* @test_extr, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
