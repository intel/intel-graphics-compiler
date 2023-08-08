;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"

declare <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.i64(<1 x i1>, i8, i8, i8, i8, i8, i64, i64, i16, i32, <32 x i32>) #4

; CHECK-LABEL: @test_fold_cast
define <32 x i32> @test_fold_cast(i8 addrspace(1)* align 8 %a) {
entry:
  %bitcast = bitcast i8 addrspace(1)* %a to double addrspace(1)*
  ; CHECK: %pti = ptrtoint i8 addrspace(1)* %a to i64
  %pti = ptrtoint double addrspace(1)* %bitcast to i64
  %res = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, i8 0, i8 0, i64 0, i64 %pti, i16 1, i32 0, <32 x i32> undef)
  ret <32 x i32> %res
}

; CHECK-LABEL: @test_fold_itp
define <32 x i32> @test_fold_itp(i64 %a) {
entry:
  %itp = inttoptr i64 %a to double addrspace(1)*
  %pti = ptrtoint double addrspace(1)* %itp to i64
  ; CHECK: %res = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, i8 0, i8 0, i64 0, i64 %a, i16 1, i32 0, <32 x i32> undef)
  %res = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, i8 0, i8 0, i64 0, i64 %pti, i16 1, i32 0, <32 x i32> undef)
  ret <32 x i32> %res
}

; CHECK-LABEL: @test_fold_cast_gep_pti
define <32 x i32> @test_fold_cast_gep_pti(i8 addrspace(1)* align 8 %a) {
entry:
  %bitcast = bitcast i8 addrspace(1)* %a to double addrspace(1)*
  ; CHECK: [[PTI:%[^ ]+]] = ptrtoint i8 addrspace(1)* %a to i64
  ; CHECK: %pti = add i64 [[PTI]], 1024
  %gep = getelementptr double, double addrspace(1)* %bitcast, i32 128
  %pti = ptrtoint double addrspace(1)* %gep to i64
  %res = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, i8 0, i8 0, i64 0, i64 %pti, i16 1, i32 0, <32 x i32> undef)
  ret <32 x i32> %res
}
