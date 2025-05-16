;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey DisableCodeScheduling=0 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:         --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 --regkey CodeSchedulingForceMWOnly=1 \
; RUN:         --igc-code-scheduling -S %s 2>&1 | FileCheck %s


; Check that the lifetime end intrinsic is not moved before the last use
; and lifetime start is not moved after the first use of the alloca

define spir_kernel void @test_lifetime() {
; CHECK-LABEL: @test_lifetime(
; CHECK:       entry:
; CHECK:         [[MAIN_ALLOCA:%.*]] = alloca [16 x i32], align 16
; CHECK:         [[BC:%.*]] = bitcast ptr [[MAIN_ALLOCA]] to ptr
; CHECK:         call void @llvm.lifetime.start.p0(i64 64, ptr [[BC]])
; CHECK:         [[BC32:%.*]] = bitcast ptr [[MAIN_ALLOCA]] to ptr
; CHECK:         [[BC32_INT:%.*]] = ptrtoint ptr [[BC32]] to i64
; CHECK:         [[PTR3_INT:%.*]] = add i64 [[BC32_INT]], [[OFFSET3:%.*]]
; CHECK:         [[PTR3:%.*]] = inttoptr i64 [[PTR3_INT]] to ptr
; CHECK:         [[PTR3_I16:%.*]] = bitcast ptr [[PTR3]] to ptr
; CHECK:         [[LOAD:%.*]] = load [16 x i32], ptr [[MAIN_ALLOCA]], align 16
; CHECK:         store i32 42, ptr [[PTR3]], align 4
; CHECK:         [[VAL1:%.*]] = load i16, ptr [[PTR3_I16]], align 2
; CHECK:         [[PTR5_INT:%.*]] = add i64 [[BC32_INT]], [[OFFSET5:%.*]]
; CHECK:         [[PTR5:%.*]] = inttoptr i64 [[PTR5_INT]] to ptr

; CHECK:         store i16 [[VAL1]], ptr [[PTR5]], align 2
; CHECK:         [[VAL2:%.*]] = load i16, ptr [[PTR5]], align 2

; CHECK:         store i16 [[A5:%.*]], ptr [[PTR5]], align 2
; CHECK:         call void @llvm.lifetime.end.p0(i64 64, ptr [[BC]])

; CHECK:         ret void
;

entry:
  %main_alloca = alloca [16 x i32], align 16
  %some_arith = add i32 1, 2
  %bc = bitcast [16 x i32]* %main_alloca to i8*
  call void @llvm.lifetime.start.p0i8(i64 64, i8* %bc)
  %load = load [16 x i32], [16 x i32]* %main_alloca, align 16

  ; Lowered GEP for index 3
  %bc32 = bitcast [16 x i32]* %main_alloca to i32*
  %bc32_int = ptrtoint i32* %bc32 to i64
  %offset3 = mul i64 3, 4
  %ptr3_int = add i64 %bc32_int, %offset3
  %ptr3 = inttoptr i64 %ptr3_int to i32*
  store i32 42, i32* %ptr3, align 4

  %ptr3_i16 = bitcast i32* %ptr3 to i16*
  %val1 = load i16, i16* %ptr3_i16, align 2

  ; Random arithmetic, independent of %dpas_res
  %a1 = add i16 %val1, 123

  ; DPAS call with new signature (removed i1 argument)
  %vec = insertelement <8 x i16> undef, i16 %val1, i32 0
  %dpas_res = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float> zeroinitializer, <8 x i16> %vec, <8 x i32> zeroinitializer, i32 1, i32 2, i32 3, i32 4, i1 false)

  %offset5 = mul i64 5, 4
  %ptr5_int = add i64 %bc32_int, %offset5
  %ptr5 = inttoptr i64 %ptr5_int to i16*
  store i16 %val1, i16* %ptr5, align 2
  %val2 = load i16, i16* %ptr5, align 2

  %a2 = mul i16 %val2, 7
  %a3 = xor i16 %a1, %a2
  %a4 = sub i16 %a3, 42
  %a5 = or i16 %a4, %val1

  ; store a5
  store i16 %a5, i16* %ptr5, align 2

  call void @llvm.lifetime.end.p0i8(i64 64, i8* %bc)
  ret void
}

declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture)
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture)

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
