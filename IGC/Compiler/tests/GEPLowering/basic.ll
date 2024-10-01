;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; ------------------------------------------------
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -debugify --igc-gep-lowering -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GEPLowering
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

%struct._test = type { i32, <4 x float>, [2 x i64] }

define spir_kernel void @test_gep(ptr byval(%struct._test) %src, i32 %iptr) {
; CHECK-LABEL: @test_gep(
; CHECK:    [[TEMP:%.*]] = alloca [[STRUCT__TEST:%.*]], align 16
; CHECK:    [[TMP1:%.*]] = ptrtoint ptr [[TEMP]] to i32
; CHECK:    [[TMP2:%.*]] = add i32 [[TMP1]], 48
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP2]], 16
; CHECK:    [[TMP4:%.*]] = inttoptr i32 [[TMP3]] to ptr
; CHECK:    [[TMP5:%.*]] = load <4 x float>, ptr [[TMP4]], align 16
; CHECK:    [[PTR:%.*]] = inttoptr i32 [[IPTR:%.*]] to ptr
; CHECK:    [[TMP6:%.*]] = add i32 [[IPTR]], 8
; CHECK:    [[TMP7:%.*]] = inttoptr i32 [[TMP6]] to ptr
; CHECK:    [[TMP8:%.*]] = load i32, ptr [[TMP7]], align 16
; CHECK:    ret void
;
  %temp = alloca %struct._test, align 16
  %a = getelementptr inbounds %struct._test, ptr %temp, i32 1, i32 1
  %1 = load <4 x float>, ptr %a, align 16
  %ptr = inttoptr i32 %iptr to ptr
  %b = getelementptr inbounds i32, ptr %ptr, i32 2
  %2 = load i32, ptr %b, align 16
  ret void
}

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}

!0 = !{ptr @test_gep, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
