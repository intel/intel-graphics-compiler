;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s

define i8* @test_scalar_0(i1 %cond, i8* %left, i8* %right) {
  ; CHECK: [[IS0T:%.+]] = ptrtoint i8* %left to i64
  ; CHECK-NEXT: [[IS0F:%.+]] = ptrtoint i8* %right to i64
  ; CHECK-NEXT: [[IV0T:%.+iv32cast[0-9]*]] = bitcast i64 [[IS0T]] to <2 x i32>
  ; CHECK-NEXT: [[LOS0T:%.+LoSplit[0-9]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[IV0T]], i32 0, i32 1, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT: [[HIS0T:%.+HiSplit[0-9]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[IV0T]], i32 0, i32 1, i32 2, i16 4, i32 undef)
  ; CHECK-NEXT: [[IV0F:%.+iv32cast[0-9]*]] = bitcast i64 [[IS0F]] to <2 x i32>
  ; CHECK-NEXT: [[LOS0F:%.+LoSplit[0-9]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[IV0F]], i32 0, i32 1, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT: [[HIS0F:%.+HiSplit[0-9]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[IV0F]], i32 0, i32 1, i32 2, i16 4, i32 undef)
  ; CHECK-NEXT: [[SEL0LO:%.+lo[0-9]*]] = select i1 %cond, <1 x i32> [[LOS0T]], <1 x i32> [[LOS0F]]
  ; CHECK-NEXT: [[SEL0HI:%.+hi[0-9]*]] = select i1 %cond, <1 x i32> [[HIS0T]], <1 x i32> [[HIS0F]]
  ; CHECK-NEXT: [[P_JOIN0:%.+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> undef, <1 x i32> [[SEL0LO]], i32 0, i32 1, i32 2, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[JOINED0:%.+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> [[P_JOIN0]], <1 x i32> [[SEL0HI]], i32 0, i32 1, i32 2, i16 4, i32 undef, i1 true)
  ; CHECK-NEXT: [[VCAST0:%.+]] = bitcast <2 x i32> [[JOINED0]] to <1 x i64>
  ; CHECK-NEXT: [[SCAST0:%.+]] = bitcast <1 x i64> [[VCAST0]] to i64
  ; CHECK-NEXT: [[PTRCAST0:%.+]] = inttoptr i64 [[SCAST0]] to i8*
  ; CHECL-NEXT: ret i8* [[PTRCAST0]]
  %res = select i1 %cond, i8* %left, i8* %right
  ret i8* %res
}

define i8 addrspace(1)* @test_scalar_1(i1 %cond, i8 addrspace(1)* %left, i8 addrspace(1)* %right) {
  ; CHECK: [[IS1T:%.+]] = ptrtoint i8 addrspace(1)* %left to i64
  ; CHECK-NEXT: [[IS1F:%.+]] = ptrtoint i8 addrspace(1)* %right to i64
  ; CHECK-NEXT: [[IV1T:%.+iv32cast[0-9]*]] = bitcast i64 [[IS1T]] to <2 x i32>
  ; CHECK-NEXT: [[LOS1T:%.+LoSplit[0-9]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[IV1T]], i32 0, i32 1, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT: [[HIS1T:%.+HiSplit[0-9]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[IV1T]], i32 0, i32 1, i32 2, i16 4, i32 undef)
  ; CHECK-NEXT: [[IV1F:%.+iv32cast[0-9]*]] = bitcast i64 [[IS1F]] to <2 x i32>
  ; CHECK-NEXT: [[LOS1F:%.+LoSplit[0-9]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[IV1F]], i32 0, i32 1, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT: [[HIS1F:%.+HiSplit[0-9]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[IV1F]], i32 0, i32 1, i32 2, i16 4, i32 undef)
  ; CHECK-NEXT: [[SEL1LO:%.+lo[0-9]*]] = select i1 %cond, <1 x i32> [[LOS1T]], <1 x i32> [[LOS1F]]
  ; CHECK-NEXT: [[SEL1HI:%.+hi[0-9]*]] = select i1 %cond, <1 x i32> [[HIS1T]], <1 x i32> [[HIS1F]]
  ; CHECK-NEXT: [[P_JOIN1:%.+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> undef, <1 x i32> [[SEL1LO]], i32 0, i32 1, i32 2, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[JOINED1:%.+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> [[P_JOIN1]], <1 x i32> [[SEL1HI]], i32 0, i32 1, i32 2, i16 4, i32 undef, i1 true)
  ; CHECK-NEXT: [[VCAST1:%.+]] = bitcast <2 x i32> [[JOINED1]] to <1 x i64>
  ; CHECK-NEXT: [[SCAST1:%.+]] = bitcast <1 x i64> [[VCAST1]] to i64
  ; CHECK-NEXT: [[PTRCAST1:%.+]] = inttoptr i64 [[SCAST1]] to i8 addrspace(1)*
  ; CHECL-NEXT: ret i8 addrspace(1)* [[PTRCAST1]]
  %res = select i1 %cond, i8 addrspace(1)* %left, i8 addrspace(1)* %right
  ret i8 addrspace(1)* %res
}

define <8 x i8*> @test_vector(<8 x i1> %cond, <8 x i8*> %left, <8 x i8*> %right) {
  ; CHECK: [[IS2T:%.+]] = ptrtoint <8 x i8*> %left to <8 x i64>
  ; CHECK-NEXT: [[IS2F:%.+]] = ptrtoint <8 x i8*> %right to <8 x i64>
  ; CHECK-NEXT: [[IV2T:%.+iv32cast[0-9]*]] = bitcast <8 x i64> [[IS2T]] to <16 x i32>
  ; CHECK-NEXT: [[LOS2T:%.+LoSplit[0-9]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[IV2T]], i32 0, i32 8, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT: [[HIS2T:%.+HiSplit[0-9]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[IV2T]], i32 0, i32 8, i32 2, i16 4, i32 undef)
  ; CHECK-NEXT: [[IV2F:%.+iv32cast[0-9]*]] = bitcast <8 x i64> [[IS2F]] to <16 x i32>
  ; CHECK-NEXT: [[LOS2F:%.+LoSplit[0-9]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[IV2F]], i32 0, i32 8, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT: [[HIS2F:%.+HiSplit[0-9]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[IV2F]], i32 0, i32 8, i32 2, i16 4, i32 undef)
  ; CHECK-NEXT: [[SEL2LO:%.+lo[0-9]*]] = select <8 x i1> %cond, <8 x i32> [[LOS2T]], <8 x i32> [[LOS2F]]
  ; CHECK-NEXT: [[SEL2HI:%.+hi[0-9]*]] = select <8 x i1> %cond, <8 x i32> [[HIS2T]], <8 x i32> [[HIS2F]]
  ; CHECK-NEXT: [[P_JOIN2:%.+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[SEL2LO]], i32 0, i32 8, i32 2, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[JOINED2:%.+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[P_JOIN2]], <8 x i32> [[SEL2HI]], i32 0, i32 8, i32 2, i16 4, i32 undef, i1 true)
  ; CHECK-NEXT: [[VCAST2:%.+]] = bitcast <16 x i32> [[JOINED2]] to <8 x i64>
  ; CHECK-NEXT: [[PTRCAST2:%.+]] = inttoptr <8 x i64> [[VCAST2]] to <8 x i8*>
  ; CHECL-NEXT: ret <8 x i8*> [[PTRCAST2]]
  %res = select <8 x i1> %cond, <8 x i8*> %left, <8 x i8*> %right
  ret <8 x i8*> %res
}

; CHECK-LABEL: @test_constexpr
define float addrspace(1)* @test_constexpr(i1 %cond) {
  ; CHECK: [[SEL_LO:%[^ ]+]] = select i1 %cond, <1 x i32> zeroinitializer, <1 x i32> <i32 ptrtoint (float addrspace(1)* addrspacecast (float addrspace(4)* null to float addrspace(1)*) to i32)>
  ; CHECK: [[SEL_HI:%[^ ]+]] = select i1 %cond, <1 x i32> zeroinitializer, <1 x i32> <i32 trunc (i64 lshr (i64 ptrtoint (float addrspace(1)* addrspacecast (float addrspace(4)* null to float addrspace(1)*) to i64), i64 32) to i32)>
  ; CHECK: [[PART_JOIN:%[^ ]+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> undef, <1 x i32> [[SEL_LO]], i32 0, i32 1, i32 2, i16 0, i32 undef, i1 true)
  ; CHECK: [[JOIN:%[^ ]+]] = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> [[PART_JOIN]], <1 x i32> [[SEL_HI]], i32 0, i32 1, i32 2, i16 4, i32 undef, i1 true)
  ; CHECK: [[VCAST:%[^ ]+]] = bitcast <2 x i32> [[JOIN]] to <1 x i64>
  ; CHECK: [[ICAST:%[^ ]+]] = bitcast <1 x i64> [[VCAST]] to i64
  ; CHECK: [[ITP:%[^ ]+]] = inttoptr i64 [[ICAST]] to float addrspace(1)*
  ; CHECK: ret float addrspace(1)* [[ITP]]
  %res = select i1 %cond, float addrspace(1)* null, float addrspace(1)* addrspacecast(float addrspace(4)* null to float addrspace(1)*)
  ret float addrspace(1)* %res
}
