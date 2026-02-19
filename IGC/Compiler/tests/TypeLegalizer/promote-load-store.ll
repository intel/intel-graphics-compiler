;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-type-legalizer -S < %s | FileCheck %s

; Test checks illegal integer promotion for stores and loads

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"


define void @test_load() {
; CHECK-LABEL: @test_load(
; CHECK:    [[TMP1:%.*]] = alloca i10, align 4
; CHECK:    [[DOTPROMOTEDALLOCA:%.*]] = alloca i16, align 4
; CHECK:    [[DOTPROMOTEDLOAD:%.*]] = load i16, i16* [[DOTPROMOTEDALLOCA]], align 4
; CHECK:    [[TMP2:%.*]] = trunc i16 [[DOTPROMOTEDLOAD]] to i8
; CHECK:    call void @use.i8(i8 [[TMP2]])
; CHECK:    ret void
;
  %1 = alloca i10, align 4
  %2 = load i10, i10* %1, align 4
  %3 = trunc i10 %2 to i8
  call void @use.i8(i8 %3)
  ret void
}

define void @test_store() {
; CHECK-LABEL: @test_store(
; CHECK:    [[TMP1:%.*]] = alloca i10, align 4
; CHECK:    [[DOTPROMOTEDALLOCA:%.*]] = alloca i16, align 4
; CHECK:    [[DOTPTRCAST:%.*]] = bitcast i10* [[TMP1]] to i8*
; CHECK:    [[DOTPTRCAST_PTRCAST:%.*]] = bitcast i8* [[DOTPTRCAST]] to i16*
; CHECK:    store i16 13, i16* [[DOTPTRCAST_PTRCAST]], align 4
; CHECK:    ret void
;
  %1 = alloca i10, align 4
  store i10 13, i10* %1, align 4
  ret void
}

define void @test_load_multiple(i56* %a) {
; CHECK-LABEL: @test_load_multiple(
; CHECK:    [[A_PTRCAST:%.*]] = bitcast i56* [[A:%.*]] to i8*
; CHECK:    [[A_PTRCAST_PTRCAST:%.*]] = bitcast i8* [[A_PTRCAST]] to i32*
; CHECK:    [[DOTPROMOTE0:%.*]] = load i32, i32* [[A_PTRCAST_PTRCAST]], align 4
; CHECK:    [[DOTPROMOTE0_ZEXT:%.*]] = zext i32 [[DOTPROMOTE0]] to i64
; CHECK:    [[DOTPROMOTE0_ZEXT_CONCAT:%.*]] = or i64 0, [[DOTPROMOTE0_ZEXT]]
; CHECK:    [[A_PTRCAST_OFF4:%.*]] = getelementptr inbounds i8, i8* [[A_PTRCAST]], i32 4
; CHECK:    [[A_PTRCAST_OFF4_PTRCAST:%.*]] = bitcast i8* [[A_PTRCAST_OFF4]] to i16*
; CHECK:    [[DOTPROMOTE1:%.*]] = load i16, i16* [[A_PTRCAST_OFF4_PTRCAST]], align 4
; CHECK:    [[DOTPROMOTE1_ZEXT:%.*]] = zext i16 [[DOTPROMOTE1]] to i64
; CHECK:    [[DOTPROMOTE1_ZEXT_SHL:%.*]] = shl i64 [[DOTPROMOTE1_ZEXT]], 32
; CHECK:    [[DOTPROMOTE1_ZEXT_SHL_CONCAT:%.*]] = or i64 [[DOTPROMOTE0_ZEXT_CONCAT]], [[DOTPROMOTE1_ZEXT_SHL]]
; CHECK:    [[A_PTRCAST_OFF6:%.*]] = getelementptr inbounds i8, i8* [[A_PTRCAST]], i32 6
; CHECK:    [[DOTPROMOTE2:%.*]] = load i8, i8* [[A_PTRCAST_OFF6]], align 2
; CHECK:    [[DOTPROMOTE2_ZEXT:%.*]] = zext i8 [[DOTPROMOTE2]] to i64
; CHECK:    [[DOTPROMOTE2_ZEXT_SHL:%.*]] = shl i64 [[DOTPROMOTE2_ZEXT]], 48
; CHECK:    [[DOTPROMOTE2_ZEXT_SHL_CONCAT:%.*]] = or i64 [[DOTPROMOTE1_ZEXT_SHL_CONCAT]], [[DOTPROMOTE2_ZEXT_SHL]]
; CHECK:    [[TMP1:%.*]] = trunc i64 [[DOTPROMOTE2_ZEXT_SHL_CONCAT]] to i8
; CHECK:    call void @use.i8(i8 [[TMP1]])
; CHECK:    ret void
;
  %1 = load i56, i56* %a, align 4
  %2 = trunc i56 %1 to i8
  call void @use.i8(i8 %2)
  ret void
}

define void @test_store_multiple(i56* %a) {
; CHECK-LABEL: @test_store_multiple(
; CHECK:    [[A_PTRCAST:%.*]] = bitcast i56* [[A:%.*]] to i8*
; CHECK:    [[A_PTRCAST_PTRCAST:%.*]] = bitcast i8* [[A_PTRCAST]] to i32*
; CHECK:    store i32 42, i32* [[A_PTRCAST_PTRCAST]], align 4
; CHECK:    [[A_PTRCAST_OFF4:%.*]] = getelementptr inbounds i8, i8* [[A_PTRCAST]], i32 4
; CHECK:    [[A_PTRCAST_OFF4_PTRCAST:%.*]] = bitcast i8* [[A_PTRCAST_OFF4]] to i16*
; CHECK:    store i16 0, i16* [[A_PTRCAST_OFF4_PTRCAST]], align 4
; CHECK:    [[A_PTRCAST_OFF6:%.*]] = getelementptr inbounds i8, i8* [[A_PTRCAST]], i32 6
; CHECK:    store i8 0, i8* [[A_PTRCAST_OFF6]], align 2
; CHECK:    ret void
;
  store i56 42, i56* %a, align 4
  ret void
}

declare void @use.i8(i8)
