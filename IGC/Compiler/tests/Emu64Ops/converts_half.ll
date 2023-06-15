;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --platformdg2 --igc-emu64ops -S < %s 2>&1 | \
; RUN: FileCheck %s --check-prefixes=CHECK,%LLVM_10_CHECK_PREFIX%
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------

define void @test_fptoui_half(half %src) {
; CHECK-LABEL: @test_fptoui_half
; CHECK-PRE-LLVM-10: [[FPTOUI_REF:%.+]] = fptoui half %src to i32
; CHECK-LLVM-10-PLUS: [[FPTOUI:%.+]] = fptoui half %src to i32
; CHECK-LLVM-10-PLUS: [[FPTOUI_REF:%.+]] = freeze i32 [[FPTOUI]]
  %1 = fptoui half %src to i64
; CHECK: [[CMP:%.+]] = icmp eq i32 [[FPTOUI_REF]], -1
; CHECK: [[SEL:%.+]] = select i1 [[CMP]], i32 -1, i32 0
; CHECK: [[INS_ELT_LO:%.+]] = insertelement <2 x i32> undef, i32 [[FPTOUI_REF]], i32 0
; CHECK: [[INS_ELT_HI:%.+]] = insertelement <2 x i32> [[INS_ELT_LO]], i32 [[SEL]], i32 1
; CHECK: [[CAST:%.+]] = bitcast <2 x i32> [[INS_ELT_HI]] to i64
; CHECK: call void @use.i64(i64 [[CAST]])
  call void @use.i64(i64 %1)
  ret void
}

define void @test_fptosi_half(half %src) {
; CHECK-LABEL: @test_fptosi_half
; CHECK-PRE-LLVM-10: [[FPTOSI_REF:%.+]] = fptosi half %src to i32
; CHECK-LLVM-10-PLUS: [[FPTOSI:%.+]] = fptosi half %src to i32
; CHECK-LLVM-10-PLUS: [[FPTOSI_REF:%.+]] = freeze i32 [[FPTOSI]]
  %1 = fptosi half %src to i64
; CHECK: [[FPTOSI_INV:%.+]] = ashr i32 [[FPTOSI_REF]], 31
;                                                    ; 2^31 - 1
; CHECK: [[CMP_MAX:%.+]] = icmp eq i32 [[FPTOSI_REF]], 2147483647
; CHECK: [[MAYBE_MAX:%.+]] = select i1 [[CMP_MAX]], i32 [[FPTOSI_REF]], i32 [[FPTOSI_INV]]
; CHECK: [[MAYBE_MIN:%.+]] = select i1 [[CMP_MAX]], i32 -1, i32 [[FPTOSI_REF]]
;                                                   ; -2^31
; CHECK: [[CMP_MIN:%.+]] = icmp eq i32 [[MAYBE_MIN]], -2147483648
; CHECK: [[LOW_BITS:%.+]] = select i1 [[CMP_MIN]], i32 [[MAYBE_MIN]], i32 [[MAYBE_MAX]]
; CHECK: [[HIGH_BITS:%.+]] = select i1 [[CMP_MIN]], i32 0, i32 [[MAYBE_MIN]]
; CHECK: [[INS_ELT_LO:%.+]] = insertelement <2 x i32> undef, i32 [[HIGH_BITS]], i32 0
; CHECK: [[INS_ELT_HI:%.+]] = insertelement <2 x i32> [[INS_ELT_LO]], i32 [[LOW_BITS]], i32 1
; CHECK: [[CAST:%.+]] = bitcast <2 x i32> [[INS_ELT_HI]] to i64
; CHECK: call void @use.i64(i64 [[CAST]])
  call void @use.i64(i64 %1)
  ret void
}

declare void @use.i64(i64)

!igc.functions = !{!0, !3}

!0 = !{void (half)* @test_fptoui_half, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{void (half)* @test_fptosi_half, !1}
