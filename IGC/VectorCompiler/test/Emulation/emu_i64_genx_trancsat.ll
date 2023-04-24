;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s

; COM: these tests just check that there is no compilation/asserts failures

declare <2 x i64>  @llvm.genx.uutrunc.sat.v2i64.v2i8(<2 x i8>)
declare <2 x i64> @llvm.genx.uutrunc.sat.v2i64.v2i16(<2 x i16>)
declare <2 x i64> @llvm.genx.uutrunc.sat.v2i64.v2i32(<2 x i32>)
declare <2 x i8>   @llvm.genx.uutrunc.sat.v2i8.v2i64(<2 x i64>)
declare <2 x i16> @llvm.genx.uutrunc.sat.v2i16.v2i64(<2 x i64>)
declare <2 x i32> @llvm.genx.uutrunc.sat.v2i32.v2i64(<2 x i64>)
declare <2 x i64> @llvm.genx.uutrunc.sat.v2i64.v2i64(<2 x i64>)

declare <2 x i64>  @llvm.genx.ustrunc.sat.v2i64.v2i8(<2 x i8>)
declare <2 x i64> @llvm.genx.ustrunc.sat.v2i64.v2i16(<2 x i16>)
declare <2 x i64> @llvm.genx.ustrunc.sat.v2i64.v2i32(<2 x i32>)
declare <2 x i8>   @llvm.genx.ustrunc.sat.v2i8.v2i64(<2 x i64>)
declare <2 x i16> @llvm.genx.ustrunc.sat.v2i16.v2i64(<2 x i64>)
declare <2 x i32> @llvm.genx.ustrunc.sat.v2i32.v2i64(<2 x i64>)
declare <2 x i64> @llvm.genx.ustrunc.sat.v2i64.v2i64(<2 x i64>)

declare <2 x i64>  @llvm.genx.sutrunc.sat.v2i64.v2i8(<2 x i8>)
declare <2 x i64> @llvm.genx.sutrunc.sat.v2i64.v2i16(<2 x i16>)
declare <2 x i64> @llvm.genx.sutrunc.sat.v2i64.v2i32(<2 x i32>)
declare <2 x i8>   @llvm.genx.sutrunc.sat.v2i8.v2i64(<2 x i64>)
declare <2 x i16> @llvm.genx.sutrunc.sat.v2i16.v2i64(<2 x i64>)
declare <2 x i32> @llvm.genx.sutrunc.sat.v2i32.v2i64(<2 x i64>)
declare <2 x i64> @llvm.genx.sutrunc.sat.v2i64.v2i64(<2 x i64>)

declare <2 x i64>  @llvm.genx.sstrunc.sat.v2i64.v2i8(<2 x i8>)
declare <2 x i64> @llvm.genx.sstrunc.sat.v2i64.v2i16(<2 x i16>)
declare <2 x i64> @llvm.genx.sstrunc.sat.v2i64.v2i32(<2 x i32>)
declare <2 x i8>   @llvm.genx.sstrunc.sat.v2i8.v2i64(<2 x i64>)
declare <2 x i16> @llvm.genx.sstrunc.sat.v2i16.v2i64(<2 x i64>)
declare <2 x i32> @llvm.genx.sstrunc.sat.v2i32.v2i64(<2 x i64>)
declare <2 x i64> @llvm.genx.sstrunc.sat.v2i64.v2i64(<2 x i64>)

declare i64  @llvm.genx.uutrunc.sat.si64.si8(i8)
declare i64 @llvm.genx.uutrunc.sat.si64.si16(i16)
declare i64 @llvm.genx.uutrunc.sat.si64.si32(i32)
declare i8   @llvm.genx.uutrunc.sat.si8.si64(i64)
declare i16 @llvm.genx.uutrunc.sat.si16.si64(i64)
declare i32 @llvm.genx.uutrunc.sat.si32.si64(i64)
declare i64 @llvm.genx.uutrunc.sat.si64.si64(i64)

declare i64  @llvm.genx.ustrunc.sat.si64.si8(i8)
declare i64 @llvm.genx.ustrunc.sat.si64.si16(i16)
declare i64 @llvm.genx.ustrunc.sat.si64.si32(i32)
declare i8   @llvm.genx.ustrunc.sat.si8.si64(i64)
declare i16 @llvm.genx.ustrunc.sat.si16.si64(i64)
declare i32 @llvm.genx.ustrunc.sat.si32.si64(i64)
declare i64 @llvm.genx.ustrunc.sat.si64.si64(i64)

declare i64  @llvm.genx.sutrunc.sat.si64.si8(i8)
declare i64 @llvm.genx.sutrunc.sat.si64.si16(i16)
declare i64 @llvm.genx.sutrunc.sat.si64.si32(i32)
declare i8   @llvm.genx.sutrunc.sat.si8.si64(i64)
declare i16 @llvm.genx.sutrunc.sat.si16.si64(i64)
declare i32 @llvm.genx.sutrunc.sat.si32.si64(i64)
declare i64 @llvm.genx.sutrunc.sat.si64.si64(i64)

declare i64  @llvm.genx.sstrunc.sat.si64.si8(i8)
declare i64 @llvm.genx.sstrunc.sat.si64.si16(i16)
declare i64 @llvm.genx.sstrunc.sat.si64.si32(i32)
declare i8   @llvm.genx.sstrunc.sat.si8.si64(i64)
declare i16 @llvm.genx.sstrunc.sat.si16.si64(i64)
declare i32 @llvm.genx.sstrunc.sat.si32.si64(i64)
declare i64 @llvm.genx.sstrunc.sat.si64.si64(i64)

; CHECK: @test_genx_vector_trunc_sat_sanity8
; CHECK: [[USER:%u_uu[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>

define dllexport spir_kernel void @test_genx_vector_trunc_sat_sanity8(<2 x i8> %op) {
  %uuval64 = call <2 x i64> @llvm.genx.uutrunc.sat.v2i64.v2i8(<2 x i8> %op)
  %usval64 = call <2 x i64> @llvm.genx.ustrunc.sat.v2i64.v2i8(<2 x i8> %op)
  %suval64 = call <2 x i64> @llvm.genx.sutrunc.sat.v2i64.v2i8(<2 x i8> %op)
  %ssval64 = call <2 x i64> @llvm.genx.sstrunc.sat.v2i64.v2i8(<2 x i8> %op)

  %u_uuval64 = bitcast <2 x i64> %uuval64 to <2 x i64>
  %u_usval64 = bitcast <2 x i64> %usval64 to <2 x i64>
  %u_suval64 = bitcast <2 x i64> %suval64 to <2 x i64>
  %u_ssval64 = bitcast <2 x i64> %ssval64 to <2 x i64>

  ret void
}

; CHECK: @test_genx_vector_trunc_sat_sanity16
; CHECK: [[USER:%u_uu[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>

define dllexport spir_kernel void @test_genx_vector_trunc_sat_sanity16(<2 x i16> %op) {
  %uuval64 = call <2 x i64> @llvm.genx.uutrunc.sat.v2i64.v2i16(<2 x i16> %op)
  %usval64 = call <2 x i64> @llvm.genx.ustrunc.sat.v2i64.v2i16(<2 x i16> %op)
  %suval64 = call <2 x i64> @llvm.genx.sutrunc.sat.v2i64.v2i16(<2 x i16> %op)
  %ssval64 = call <2 x i64> @llvm.genx.sstrunc.sat.v2i64.v2i16(<2 x i16> %op)

  %u_uuval64 = bitcast <2 x i64> %uuval64 to <2 x i64>
  %u_usval64 = bitcast <2 x i64> %usval64 to <2 x i64>
  %u_suval64 = bitcast <2 x i64> %suval64 to <2 x i64>
  %u_ssval64 = bitcast <2 x i64> %ssval64 to <2 x i64>

  ret void
}

; CHECK: @test_genx_vector_trunc_sat_sanity32
; CHECK: [[USER:%u_uu[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>

define dllexport spir_kernel void @test_genx_vector_trunc_sat_sanity32(<2 x i32> %op) {
  %uuval64 = call <2 x i64> @llvm.genx.uutrunc.sat.v2i64.v2i32(<2 x i32> %op)
  %usval64 = call <2 x i64> @llvm.genx.ustrunc.sat.v2i64.v2i32(<2 x i32> %op)
  %suval64 = call <2 x i64> @llvm.genx.sutrunc.sat.v2i64.v2i32(<2 x i32> %op)
  %ssval64 = call <2 x i64> @llvm.genx.sstrunc.sat.v2i64.v2i32(<2 x i32> %op)

  %u_uuval64 = bitcast <2 x i64> %uuval64 to <2 x i64>
  %u_usval64 = bitcast <2 x i64> %usval64 to <2 x i64>
  %u_suval64 = bitcast <2 x i64> %suval64 to <2 x i64>
  %u_ssval64 = bitcast <2 x i64> %ssval64 to <2 x i64>

  ret void
}

; CHECK: @test_genx_vector_trunc_sat_sanity64
; CHECK: [[USER:%u_uu[^ ]+]] = bitcast <2 x i8> {{[^ ]+}} to <2 x i8>
; CHECK-NEXT: [[USER:%u_uu[^ ]+]] = bitcast <2 x i16> {{[^ ]+}} to <2 x i16>
; CHECK-NEXT: [[USER:%u_uu[^ ]+]] = bitcast <2 x i32> {{[^ ]+}} to <2 x i32>
; CHECK-NEXT: [[USER:%u_uu[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>

; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast <2 x i8> {{[^ ]+}} to <2 x i8>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast <2 x i16> {{[^ ]+}} to <2 x i16>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast <2 x i32> {{[^ ]+}} to <2 x i32>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>

; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast <2 x i8> {{[^ ]+}} to <2 x i8>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast <2 x i16> {{[^ ]+}} to <2 x i16>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast <2 x i32> {{[^ ]+}} to <2 x i32>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>

; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast <2 x i8> {{[^ ]+}} to <2 x i8>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast <2 x i16> {{[^ ]+}} to <2 x i16>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast <2 x i32> {{[^ ]+}} to <2 x i32>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast <2 x i64> {{[^ ]+}} to <2 x i64>

define dllexport spir_kernel void @test_genx_vector_trunc_sat_sanity64(<2 x i64> %op) {
  %uuval8  = call <2 x i8>   @llvm.genx.uutrunc.sat.v2i8.v2i64(<2 x i64> %op)
  %uuval16 = call <2 x i16> @llvm.genx.uutrunc.sat.v2i16.v2i64(<2 x i64> %op)
  %uuval32 = call <2 x i32> @llvm.genx.uutrunc.sat.v2i32.v2i64(<2 x i64> %op)
  %uuval64 = call <2 x i64> @llvm.genx.uutrunc.sat.v2i64.v2i64(<2 x i64> %op)

  %usval8  = call <2 x i8>   @llvm.genx.ustrunc.sat.v2i8.v2i64(<2 x i64> %op)
  %usval16 = call <2 x i16> @llvm.genx.ustrunc.sat.v2i16.v2i64(<2 x i64> %op)
  %usval32 = call <2 x i32> @llvm.genx.ustrunc.sat.v2i32.v2i64(<2 x i64> %op)
  %usval64 = call <2 x i64> @llvm.genx.ustrunc.sat.v2i64.v2i64(<2 x i64> %op)

  %suval8  = call <2 x i8>   @llvm.genx.sutrunc.sat.v2i8.v2i64(<2 x i64> %op)
  %suval16 = call <2 x i16> @llvm.genx.sutrunc.sat.v2i16.v2i64(<2 x i64> %op)
  %suval32 = call <2 x i32> @llvm.genx.sutrunc.sat.v2i32.v2i64(<2 x i64> %op)
  %suval64 = call <2 x i64> @llvm.genx.sutrunc.sat.v2i64.v2i64(<2 x i64> %op)

  %ssval8  = call <2 x i8>   @llvm.genx.sstrunc.sat.v2i8.v2i64(<2 x i64> %op)
  %ssval16 = call <2 x i16> @llvm.genx.sstrunc.sat.v2i16.v2i64(<2 x i64> %op)
  %ssval32 = call <2 x i32> @llvm.genx.sstrunc.sat.v2i32.v2i64(<2 x i64> %op)
  %ssval64 = call <2 x i64> @llvm.genx.sstrunc.sat.v2i64.v2i64(<2 x i64> %op)

  %u_uuval8  = bitcast <2 x i8>  %uuval8  to <2 x i8>
  %u_uuval16 = bitcast <2 x i16> %uuval16 to <2 x i16>
  %u_uuval32 = bitcast <2 x i32> %uuval32 to <2 x i32>
  %u_uuval64 = bitcast <2 x i64> %uuval64 to <2 x i64>

  %u_usval8  = bitcast <2 x i8>  %usval8  to <2 x i8>
  %u_usval16 = bitcast <2 x i16> %usval16 to <2 x i16>
  %u_usval32 = bitcast <2 x i32> %usval32 to <2 x i32>
  %u_usval64 = bitcast <2 x i64> %usval64 to <2 x i64>

  %u_suval8  = bitcast <2 x i8>  %suval8  to <2 x i8>
  %u_suval16 = bitcast <2 x i16> %suval16 to <2 x i16>
  %u_suval32 = bitcast <2 x i32> %suval32 to <2 x i32>
  %u_suval64 = bitcast <2 x i64> %suval64 to <2 x i64>

  %u_ssval8  = bitcast <2 x i8>  %ssval8  to <2 x i8>
  %u_ssval16 = bitcast <2 x i16> %ssval16 to <2 x i16>
  %u_ssval32 = bitcast <2 x i32> %ssval32 to <2 x i32>
  %u_ssval64 = bitcast <2 x i64> %ssval64 to <2 x i64>

  ret void
}

; CHECK: @test_genx_scalar_trunc_sat_sanity8
; CHECK: [[USER:%u_uu[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>

define dllexport spir_kernel void @test_genx_scalar_trunc_sat_sanity8(i8 %op) {
  %uuval64 = call i64 @llvm.genx.uutrunc.sat.si64.si8(i8 %op)
  %usval64 = call i64 @llvm.genx.ustrunc.sat.si64.si8(i8 %op)
  %suval64 = call i64 @llvm.genx.sutrunc.sat.si64.si8(i8 %op)
  %ssval64 = call i64 @llvm.genx.sstrunc.sat.si64.si8(i8 %op)

  %u_uuval64 = bitcast i64 %uuval64 to <1 x i64>
  %u_usval64 = bitcast i64 %usval64 to <1 x i64>
  %u_suval64 = bitcast i64 %suval64 to <1 x i64>
  %u_ssval64 = bitcast i64 %ssval64 to <1 x i64>

  ret void
}

; CHECK: @test_genx_scalar_trunc_sat_sanity16
; CHECK: [[USER:%u_uu[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>

define dllexport spir_kernel void @test_genx_scalar_trunc_sat_sanity16(i16 %op) {
  %uuval64 = call i64 @llvm.genx.uutrunc.sat.si64.si16(i16 %op)
  %usval64 = call i64 @llvm.genx.ustrunc.sat.si64.si16(i16 %op)
  %suval64 = call i64 @llvm.genx.sutrunc.sat.si64.si16(i16 %op)
  %ssval64 = call i64 @llvm.genx.sstrunc.sat.si64.si16(i16 %op)

  %u_uuval64 = bitcast i64 %uuval64 to <1 x i64>
  %u_usval64 = bitcast i64 %usval64 to <1 x i64>
  %u_suval64 = bitcast i64 %suval64 to <1 x i64>
  %u_ssval64 = bitcast i64 %ssval64 to <1 x i64>

  ret void
}

; CHECK: @test_genx_scalar_trunc_sat_sanity32
; CHECK: [[USER:%u_uu[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>

define dllexport spir_kernel void @test_genx_scalar_trunc_sat_sanity32(i32 %op) {
  %uuval64 = call i64 @llvm.genx.uutrunc.sat.si64.si32(i32 %op)
  %usval64 = call i64 @llvm.genx.ustrunc.sat.si64.si32(i32 %op)
  %suval64 = call i64 @llvm.genx.sutrunc.sat.si64.si32(i32 %op)
  %ssval64 = call i64 @llvm.genx.sstrunc.sat.si64.si32(i32 %op)

  %u_uuval64 = bitcast i64 %uuval64 to <1 x i64>
  %u_usval64 = bitcast i64 %usval64 to <1 x i64>
  %u_suval64 = bitcast i64 %suval64 to <1 x i64>
  %u_ssval64 = bitcast i64 %ssval64 to <1 x i64>

  ret void
}

; CHECK: @test_genx_scalar_trunc_sat_sanity64
; CHECK: [[USER:%u_uu[^ ]+]] = bitcast i8 {{[^ ]+}} to <1 x i8>
; CHECK-NEXT: [[USER:%u_uu[^ ]+]] = bitcast i16 {{[^ ]+}} to <1 x i16>
; CHECK-NEXT: [[USER:%u_uu[^ ]+]] = bitcast i32 {{[^ ]+}} to <1 x i32>
; CHECK-NEXT: [[USER:%u_uu[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>

; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast i8 {{[^ ]+}} to <1 x i8>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast i16 {{[^ ]+}} to <1 x i16>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast i32 {{[^ ]+}} to <1 x i32>
; CHECK-NEXT: [[USER:%u_us[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>

; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast i8 {{[^ ]+}} to <1 x i8>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast i16 {{[^ ]+}} to <1 x i16>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast i32 {{[^ ]+}} to <1 x i32>
; CHECK-NEXT: [[USER:%u_su[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>

; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast i8 {{[^ ]+}} to <1 x i8>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast i16 {{[^ ]+}} to <1 x i16>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast i32 {{[^ ]+}} to <1 x i32>
; CHECK-NEXT: [[USER:%u_ss[^ ]+]] = bitcast i64 {{[^ ]+}} to <1 x i64>

define dllexport spir_kernel void @test_genx_scalar_trunc_sat_sanity64(i64 %op) {
  %uuval8  = call i8   @llvm.genx.uutrunc.sat.si8.si64(i64 %op)
  %uuval16 = call i16 @llvm.genx.uutrunc.sat.si16.si64(i64 %op)
  %uuval32 = call i32 @llvm.genx.uutrunc.sat.si32.si64(i64 %op)
  %uuval64 = call i64 @llvm.genx.uutrunc.sat.si64.si64(i64 %op)

  %usval8  = call i8   @llvm.genx.ustrunc.sat.si8.si64(i64 %op)
  %usval16 = call i16 @llvm.genx.ustrunc.sat.si16.si64(i64 %op)
  %usval32 = call i32 @llvm.genx.ustrunc.sat.si32.si64(i64 %op)
  %usval64 = call i64 @llvm.genx.ustrunc.sat.si64.si64(i64 %op)

  %suval8  = call i8   @llvm.genx.sutrunc.sat.si8.si64(i64 %op)
  %suval16 = call i16 @llvm.genx.sutrunc.sat.si16.si64(i64 %op)
  %suval32 = call i32 @llvm.genx.sutrunc.sat.si32.si64(i64 %op)
  %suval64 = call i64 @llvm.genx.sutrunc.sat.si64.si64(i64 %op)

  %ssval8  = call i8   @llvm.genx.sstrunc.sat.si8.si64(i64 %op)
  %ssval16 = call i16 @llvm.genx.sstrunc.sat.si16.si64(i64 %op)
  %ssval32 = call i32 @llvm.genx.sstrunc.sat.si32.si64(i64 %op)
  %ssval64 = call i64 @llvm.genx.sstrunc.sat.si64.si64(i64 %op)

  %u_uuval8  = bitcast i8  %uuval8  to <1 x i8>
  %u_uuval16 = bitcast i16 %uuval16 to <1 x i16>
  %u_uuval32 = bitcast i32 %uuval32 to <1 x i32>
  %u_uuval64 = bitcast i64 %uuval64 to <1 x i64>

  %u_usval8  = bitcast i8  %usval8  to <1 x i8>
  %u_usval16 = bitcast i16 %usval16 to <1 x i16>
  %u_usval32 = bitcast i32 %usval32 to <1 x i32>
  %u_usval64 = bitcast i64 %usval64 to <1 x i64>

  %u_suval8  = bitcast i8  %suval8  to <1 x i8>
  %u_suval16 = bitcast i16 %suval16 to <1 x i16>
  %u_suval32 = bitcast i32 %suval32 to <1 x i32>
  %u_suval64 = bitcast i64 %suval64 to <1 x i64>

  %u_ssval8  = bitcast i8  %ssval8  to <1 x i8>
  %u_ssval16 = bitcast i16 %ssval16 to <1 x i16>
  %u_ssval32 = bitcast i32 %ssval32 to <1 x i32>
  %u_ssval64 = bitcast i64 %ssval64 to <1 x i64>

  ret void
}
