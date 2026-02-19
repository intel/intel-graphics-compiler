;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-break-const-expr -S < %s | FileCheck %s
; run: igc_opt -debugify -igc-break-const-expr -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; BreakConstantExpr
; ------------------------------------------------

; Debug-info check fails
;
; COM: check-not WARNING
; COM: check CheckModuleDebugify: PASS

@x = addrspace(2) constant i32 3, align 4
@y = addrspace(1) constant i32 13, align 4

define spir_kernel void @test_breakexpr(i32* %dst, i32 %isrc, float %fsrc) {
; CHECK-LABEL: @test_breakexpr(
; CHECK:    [[TMP1:%.*]] = getelementptr inbounds i32, i32 addrspace(2)* @x, i32 2
; CHECK:    [[TMP2:%.*]] = ptrtoint i32 addrspace(2)* [[TMP1]] to i32
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP2]], 1
; CHECK:    [[TMP4:%.*]] = insertelement <2 x i32> undef, i32 [[TMP3]], i32 0
; CHECK:    [[TMP5:%.*]] = ptrtoint i32 addrspace(1)* @y to i32
; CHECK:    [[TMP6:%.*]] = insertelement <2 x i32> [[TMP4]], i32 [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <2 x i32> [[TMP6]], i32 1
; CHECK:    call void @use.i32(i32 [[TMP7]])
; CHECK:    ret void
;
  %1 = extractelement <2 x i32> <i32 add (i32 ptrtoint (i32 addrspace(2)* getelementptr inbounds (i32, i32 addrspace(2)* @x, i32 2) to i32), i32 1), i32 ptrtoint (i32 addrspace(1)* @y to i32)>, i32 1
  call void @use.i32(i32 %1)
  ret void
}

declare void @use.i32(i32)
