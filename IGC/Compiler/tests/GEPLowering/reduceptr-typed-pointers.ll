;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; ------------------------------------------------
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-gep-lowering -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GEPLowering
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_gep1(i32 %iptr) {
; CHECK-LABEL: @test_gep1(
; CHECK:    [[TMP1:%.*]] = sext i32 [[IPTR:%.*]] to i64
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], 13
; CHECK:    [[PTR:%.*]] = inttoptr i32 [[IPTR]] to i64*
; CHECK:    [[TMP3:%.*]] = add i32 [[IPTR]], 13
; CHECK:    [[TMP4:%.*]] = shl i32 [[TMP3]], 3
; CHECK:    [[TMP5:%.*]] = add i32 [[IPTR]], [[TMP4]]
; CHECK:    [[TMP6:%.*]] = inttoptr i32 [[TMP5]] to i64*
; CHECK:    [[RES:%.*]] = load i64, i64* [[TMP6]]
; CHECK:    store i64 [[TMP1]], i64* [[TMP6]]
; CHECK:    ret void
;
  %1 = sext i32 %iptr to i64
  %2 = add i64 %1, 13
  %ptr = inttoptr i32 %iptr to i64*
  %gep = getelementptr inbounds i64, i64* %ptr, i64 %2
  %res = load i64, i64* %gep
  store i64 %1, i64* %gep
  ret void
}

define spir_kernel void @test_gep2(i32 %iptr) {
; CHECK-LABEL: @test_gep2(
; CHECK:    [[TMP1:%.*]] = add i32 [[IPTR:%.*]], 42
; CHECK:    [[TMP2:%.*]] = sext i32 [[TMP1]] to i64
; CHECK:    [[PTR:%.*]] = inttoptr i32 [[IPTR]] to i8 addrspace(2)*
; CHECK:    [[TMP3:%.*]] = ptrtoint i8 addrspace(2)* [[PTR]] to i64
; CHECK:    [[TMP4:%.*]] = add i32 0, [[IPTR]]
; CHECK:    [[TMP5:%.*]] = add i32 [[TMP4]], 42
; CHECK:    [[TMP6:%.*]] = sext i32 [[TMP5]] to i64
; CHECK:    [[TMP7:%.*]] = add i64 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = inttoptr i64 [[TMP7]] to i8 addrspace(2)*
; CHECK:    [[RES:%.*]] = load i8, i8 addrspace(2)* [[TMP8]]
; CHECK:    ret void
;
  %1 = add i32 %iptr, 42
  %2 = sext i32 %1 to i64
  %ptr = inttoptr i32 %iptr to i8 addrspace(2)*
  %gep = getelementptr inbounds i8, i8 addrspace(2)* %ptr, i64 %2
  %res = load i8, i8 addrspace(2)* %gep
  ret void
}

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0, !4}

!0 = !{void (i32)* @test_gep1, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{void (i32)* @test_gep2, !1}
