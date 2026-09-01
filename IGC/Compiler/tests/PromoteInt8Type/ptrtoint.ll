;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt %if llvm-16-plus %{--opaque-pointers%} %else %{--typed-pointers%} -enable-debugify --igc-promoteint8type -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type : ptrtoint
; ------------------------------------------------
; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PAS
define i8 @test_ptrtoint(i8* %src) {
; CHECK-LABEL: @test_ptrtoint(
; CHECK:    [[P2B:%.*]] = ptrtoint {{(i8\*|ptr)}} %src to i8
; CHECK:    [[B2S:%.*]] = sext i8 [[P2B]] to i16
; CHECK:    [[AND:%.*]] = and i16 [[B2S]], 7
; CHECK:    [[TRUNC:%.*]] = trunc i16 [[AND]] to i8
; CHECK:    ret i8 [[TRUNC]]

  %1 = ptrtoint i8* %src to i8
  %2 = and i8 %1, 7
  ret i8 %2
}
