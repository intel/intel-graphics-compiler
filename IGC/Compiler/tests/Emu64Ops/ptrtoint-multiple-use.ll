;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --platformdg2 --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------

; Checks if in case of multiple uses of same pointer Emu64Ops creates only one
; insertelement -> insertelement -> bitcast combination.
; It used to create new such combination for each use.

define void @test_ptrtoint(ptr %src) {
; CHECK:      {{.*}} = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p0{{.*}}
; CHECK:      [[BC:%[A-z0-9.]*]] = bitcast <2 x i32> {{.*}} to i64
; CHECK:      call void @use.i64(i64 [[BC]])
; CHECK-NEXT: call void @use.i64(i64 [[BC]])
  %1 = ptrtoint ptr %src to i64
  call void @use.i64(i64 %1)
  call void @use.i64(i64 %1)
  ret void
}

declare void @use.i1(i1)
declare void @use.i16(i16)
declare void @use.i64(i64)
declare void @use.4i16(<4 x i16>)

!igc.functions = !{!0}

!0 = !{void (ptr)* @test_ptrtoint, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
