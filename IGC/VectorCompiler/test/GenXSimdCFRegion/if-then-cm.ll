;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -simdcf-region -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------
; Check that simdcf-region algorithm apply here

; CHECK: gen_simdcf
; CHECK: llvm.genx.simdcf.goto
; CHECK: llvm.genx.simdcf.goto
; CHECK: llvm.genx.simdcf.goto
; CHECK: declare {{.*}}goto
define spir_kernel void @"gen_simdcf<char>"() {
entry:
  %allany = tail call i1 @llvm.genx.any.v32i1(<32 x i1> undef)
  br i1 %allany, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %merge = select <32 x i1> undef, <32 x i8> zeroinitializer, <32 x i8> zeroinitializer
  br label %if.end

if.end:                                           ; preds = %if.then, %entry
  br label %if.end239

if.end239:                                        ; preds = %if.end
  %allany254 = tail call i1 @llvm.genx.any.v32i1(<32 x i1> undef)
  br i1 %allany254, label %if.then255, label %if.end370

if.then255:                                       ; preds = %if.end239
  %merge346 = select <32 x i1> undef, <32 x i8> zeroinitializer, <32 x i8> zeroinitializer
  br label %if.end370

if.end370:                                        ; preds = %if.then255, %if.end239
  %allany373 = tail call i1 @llvm.genx.any.v32i1(<32 x i1> undef)
  br i1 %allany373, label %if.then374, label %if.end489

if.then374:                                       ; preds = %if.end370
  %merge393 = select <32 x i1> undef, <32 x i8> zeroinitializer, <32 x i8> zeroinitializer
  br label %if.end489

if.end489:                                        ; preds = %if.then374, %if.end370
  ret void
}

declare i1 @llvm.genx.any.v32i1(<32 x i1>)
