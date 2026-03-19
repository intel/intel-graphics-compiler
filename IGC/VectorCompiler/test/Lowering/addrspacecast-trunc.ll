;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLP \
; RUN:   -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLP \
; RUN:   -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG \
; RUN:   -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG \
; RUN:   -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC \
; RUN:   -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC \
; RUN:   -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
;
; Test that GenXLowering decomposes a size-reducing addrspacecast (64-bit to
; 32-bit pointer) into ptrtoint + trunc + inttoptr. The trunc is subsequently
; lowered to bitcast + rdregion, enabling GenXLegalization to split wide
; operations into legal chunks that respect GRF boundary constraints.
;
; Equal-size addrspacecasts must remain unchanged.

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"

; Scalar: addrspacecast is decomposed, trunc is further lowered to bitcast+rdregion.
; CHECK-LABEL: @test_scalar_trunc_as4_to_as3
; CHECK-TYPED-PTRS-NEXT: %[[P2I:[^ ]+]] = ptrtoint i8 addrspace(4)* %src to i64
; CHECK-OPAQUE-PTRS-NEXT: %[[P2I:[^ ]+]] = ptrtoint ptr addrspace(4) %src to i64
; CHECK-NEXT: %[[BC:[^ ]+]] = bitcast i64 %[[P2I]] to <2 x i32>
; CHECK-NEXT: %[[RD:[^ ]+]] = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %[[BC]], i32 2, i32 1, i32 2, i16 0, i32 undef)
; CHECK-TYPED-PTRS-NEXT: %[[I2P:[^ ]+]] = inttoptr i32 %[[RD]] to i8 addrspace(3)*
; CHECK-OPAQUE-PTRS-NEXT: %[[I2P:[^ ]+]] = inttoptr i32 %[[RD]] to ptr addrspace(3)
; CHECK-TYPED-PTRS-NEXT: ret i8 addrspace(3)* %[[I2P]]
; CHECK-OPAQUE-PTRS-NEXT: ret ptr addrspace(3) %[[I2P]]
define i8 addrspace(3)* @test_scalar_trunc_as4_to_as3(i8 addrspace(4)* %src) {
  %cast = addrspacecast i8 addrspace(4)* %src to i8 addrspace(3)*
  ret i8 addrspace(3)* %cast
}

; Vector 16: the critical case that triggered the bug (16 x i64 -> 16 x i32).
; CHECK-LABEL: @test_vec16_trunc_as4_to_as3
; CHECK-TYPED-PTRS-NEXT: %[[P2I:[^ ]+]] = ptrtoint <16 x i8 addrspace(4)*> %src to <16 x i64>
; CHECK-OPAQUE-PTRS-NEXT: %[[P2I:[^ ]+]] = ptrtoint <16 x ptr addrspace(4)> %src to <16 x i64>
; CHECK-NEXT: %[[BC:[^ ]+]] = bitcast <16 x i64> %[[P2I]] to <32 x i32>
; CHECK-NEXT: %[[RD:[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %[[BC]], i32 32, i32 16, i32 2, i16 0, i32 undef)
; CHECK-TYPED-PTRS-NEXT: %[[I2P:[^ ]+]] = inttoptr <16 x i32> %[[RD]] to <16 x i8 addrspace(3)*>
; CHECK-OPAQUE-PTRS-NEXT: %[[I2P:[^ ]+]] = inttoptr <16 x i32> %[[RD]] to <16 x ptr addrspace(3)>
; CHECK-TYPED-PTRS-NEXT: ret <16 x i8 addrspace(3)*> %[[I2P]]
; CHECK-OPAQUE-PTRS-NEXT: ret <16 x ptr addrspace(3)> %[[I2P]]
define <16 x i8 addrspace(3)*> @test_vec16_trunc_as4_to_as3(<16 x i8 addrspace(4)*> %src) {
  %cast = addrspacecast <16 x i8 addrspace(4)*> %src to <16 x i8 addrspace(3)*>
  ret <16 x i8 addrspace(3)*> %cast
}

; Vector 8: smaller vector variant.
; CHECK-LABEL: @test_vec8_trunc_as4_to_as3
; CHECK-TYPED-PTRS-NEXT: %[[P2I:[^ ]+]] = ptrtoint <8 x i8 addrspace(4)*> %src to <8 x i64>
; CHECK-OPAQUE-PTRS-NEXT: %[[P2I:[^ ]+]] = ptrtoint <8 x ptr addrspace(4)> %src to <8 x i64>
; CHECK-NEXT: %[[BC:[^ ]+]] = bitcast <8 x i64> %[[P2I]] to <16 x i32>
; CHECK-NEXT: %[[RD:[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %[[BC]], i32 16, i32 8, i32 2, i16 0, i32 undef)
; CHECK-TYPED-PTRS-NEXT: %[[I2P:[^ ]+]] = inttoptr <8 x i32> %[[RD]] to <8 x i8 addrspace(3)*>
; CHECK-OPAQUE-PTRS-NEXT: %[[I2P:[^ ]+]] = inttoptr <8 x i32> %[[RD]] to <8 x ptr addrspace(3)>
; CHECK-TYPED-PTRS-NEXT: ret <8 x i8 addrspace(3)*> %[[I2P]]
; CHECK-OPAQUE-PTRS-NEXT: ret <8 x ptr addrspace(3)> %[[I2P]]
define <8 x i8 addrspace(3)*> @test_vec8_trunc_as4_to_as3(<8 x i8 addrspace(4)*> %src) {
  %cast = addrspacecast <8 x i8 addrspace(4)*> %src to <8 x i8 addrspace(3)*>
  ret <8 x i8 addrspace(3)*> %cast
}

; Equal-size addrspacecast (AS0 64-bit -> AS4 64-bit) must NOT be decomposed.
; CHECK-LABEL: @test_equal_size_as0_to_as4
; CHECK-TYPED-PTRS-NEXT: %cast = addrspacecast i8* %src to i8 addrspace(4)*
; CHECK-OPAQUE-PTRS-NEXT: %cast = addrspacecast ptr %src to ptr addrspace(4)
; CHECK-TYPED-PTRS-NEXT: ret i8 addrspace(4)* %cast
; CHECK-OPAQUE-PTRS-NEXT: ret ptr addrspace(4) %cast
define i8 addrspace(4)* @test_equal_size_as0_to_as4(i8* %src) {
  %cast = addrspacecast i8* %src to i8 addrspace(4)*
  ret i8 addrspace(4)* %cast
}
