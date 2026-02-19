;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -enable-debugify -regkey EnableCodeAssumption=2 -igc-codeassumption -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CodeAssumption : addAssumption part
; ------------------------------------------------

; Check that assumption calls were added

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_global(i32* %dst) {
; CHECK-LABEL: @test_global(
; CHECK:    [[TMP1:%.*]] = call spir_func i64 @_Z13get_global_idj()
; CHECK:    [[ASSUMECOND:%.*]] = icmp sge i64 [[TMP1]], 0
; CHECK:    call void @llvm.assume(i1 [[ASSUMECOND]])
; CHECK:    [[TMP2:%.*]] = trunc i64 [[TMP1]] to i32
; CHECK:    [[ASSUMECOND1:%.*]] = icmp sge i32 [[TMP2]], 0
; CHECK:    call void @llvm.assume(i1 [[ASSUMECOND1]])
; CHECK:    store i32 [[TMP2]], i32* [[DST:%.*]]
; CHECK:    ret void
;
  %1 = call spir_func i64 @_Z13get_global_idj()
  %2 = trunc i64 %1 to i32
  store i32 %2, i32* %dst
  ret void
}

define spir_kernel void @test_local(i32* %dst) {
; CHECK-LABEL: @test_local(
; CHECK:    [[TMP1:%.*]] = call spir_func i64 @_Z12get_local_idj()
; CHECK:    [[ASSUMECOND:%.*]] = icmp sge i64 [[TMP1]], 0
; CHECK:    call void @llvm.assume(i1 [[ASSUMECOND]])
; CHECK:    [[TMP2:%.*]] = trunc i64 [[TMP1]] to i32
; CHECK:    [[ASSUMECOND1:%.*]] = icmp sge i32 [[TMP2]], 0
; CHECK:    call void @llvm.assume(i1 [[ASSUMECOND1]])
; CHECK:    store i32 [[TMP2]], i32* [[DST:%.*]]
; CHECK:    ret void
;
  %1 = call spir_func i64 @_Z12get_local_idj()
  %2 = trunc i64 %1 to i32
  store i32 %2, i32* %dst
  ret void
}

define spir_kernel void @test_group(i32* %dst) {
; CHECK-LABEL: @test_group(
; CHECK:    [[TMP1:%.*]] = call spir_func i64 @_Z12get_group_idj()
; CHECK:    [[ASSUMECOND:%.*]] = icmp sge i64 [[TMP1]], 0
; CHECK:    call void @llvm.assume(i1 [[ASSUMECOND]])
; CHECK:    [[TMP2:%.*]] = trunc i64 [[TMP1]] to i32
; CHECK:    [[ASSUMECOND1:%.*]] = icmp sge i32 [[TMP2]], 0
; CHECK:    call void @llvm.assume(i1 [[ASSUMECOND1]])
; CHECK:    store i32 [[TMP2]], i32* [[DST:%.*]]
; CHECK:    ret void
;
  %1 = call spir_func i64 @_Z12get_group_idj()
  %2 = trunc i64 %1 to i32
  store i32 %2, i32* %dst
  ret void
}

declare spir_func i64 @_Z13get_global_idj()
declare spir_func i64 @_Z12get_local_idj()
declare spir_func i64 @_Z12get_group_idj()

