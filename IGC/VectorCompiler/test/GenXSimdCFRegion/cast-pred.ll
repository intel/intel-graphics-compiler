;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -simdcf-region -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------
; Check that the pass does not crash when the Simd CF condition mask is
; a cast of a non-instruction value (function argument or constant).

declare i1 @llvm.genx.any.v16i1(<16 x i1>)

; CHECK-LABEL: @cast_of_arg
; CHECK-NOT: llvm.genx.simdcf.goto
; CHECK: ret void
define spir_kernel void @cast_of_arg(i16 %arg) {
entry:
  %mask = bitcast i16 %arg to <16 x i1>
  %any = tail call i1 @llvm.genx.any.v16i1(<16 x i1> %mask)
  br i1 %any, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %sel = select <16 x i1> %mask, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}

; CHECK-LABEL: @cast_of_const
; CHECK-NOT: llvm.genx.simdcf.goto
; CHECK: ret void
define spir_kernel void @cast_of_const() {
entry:
  %mask = bitcast i16 -1 to <16 x i1>
  %any = tail call i1 @llvm.genx.any.v16i1(<16 x i1> %mask)
  br i1 %any, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %sel = select <16 x i1> %mask, <16 x i32> zeroinitializer, <16 x i32> zeroinitializer
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  ret void
}
