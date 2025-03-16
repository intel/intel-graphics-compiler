;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_new_pm_typed -passes=GenXSimplify,dce -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: %opt_new_pm_opaque -passes=GenXSimplify,dce -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
;
; ------------------------------------------------
; GenXSimplify
; ------------------------------------------------
; The test verifies that GenXSimplify implements the functionality from
; LLVM-14 instcombine (the functionality was lost in LLVM-16).
; ------------------------------------------------

declare <32 x i16> @llvm.genx.wrregioni.v32i16.v17i16.i16.i1(<32 x i16>, <17 x i16>, i32, i32, i32, i16, i32, i1)

declare <17 x i16> @llvm.genx.rdregioni.v17i16.v32i16.i16(<32 x i16>, i32, i32, i32, i16, i32)

define spir_kernel void @test_binops(<17 x i16> %0) {
entry:
  %wrregion.i.i.i116 = call <32 x i16> @llvm.genx.wrregioni.v32i16.v17i16.i16.i1(<32 x i16> zeroinitializer, <17 x i16> %0, i32 0, i32 17, i32 1, i16 0, i32 undef, i1 true)
  %1 = trunc <32 x i16> %wrregion.i.i.i116 to <32 x i1>
  %call1.i.i.i.i117 = bitcast <32 x i1> %1 to i32

  %wrregion.i.i.i = call <32 x i16> @llvm.genx.wrregioni.v32i16.v17i16.i16.i1(<32 x i16> zeroinitializer, <17 x i16> %0, i32 0, i32 17, i32 1, i16 0, i32 undef, i1 true)
  %2 = trunc <32 x i16> %wrregion.i.i.i to <32 x i1>
  %call1.i.i.i.i = bitcast <32 x i1> %2 to i32

  %and.i.i = and i32 %call1.i.i.i.i, %call1.i.i.i.i117
  %or.i.i = or i32 %call1.i.i.i.i, %call1.i.i.i.i117
  %xor.i.i = xor i32 %call1.i.i.i.i, %call1.i.i.i.i117

  %3 = bitcast i32 %and.i.i to <32 x i1>
  %call.i.i73 = zext <32 x i1> %3 to <32 x i16>
  %rdr.i.i74 = call <17 x i16> @llvm.genx.rdregioni.v17i16.v32i16.i16(<32 x i16> %call.i.i73, i32 0, i32 17, i32 1, i16 0, i32 undef)
  %4 = bitcast i32 %or.i.i to <32 x i1>
  %call.i.i66 = zext <32 x i1> %4 to <32 x i16>
  %rdr.i.i67 = call <17 x i16> @llvm.genx.rdregioni.v17i16.v32i16.i16(<32 x i16> %call.i.i66, i32 0, i32 17, i32 1, i16 0, i32 undef)

; CHECK: %[[WRREG1:[^ ]+]] = call <32 x i16> @llvm.genx.wrregioni.v32i16.v17i16.i16.i1(<32 x i16> zeroinitializer, <17 x i16> {{.*}}, i32 0, i32 17, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[WRREG2:[^ ]+]] = call <32 x i16> @llvm.genx.wrregioni.v32i16.v17i16.i16.i1(<32 x i16> zeroinitializer, <17 x i16> {{.*}}, i32 0, i32 17, i32 1, i16 0, i32 undef, i1 true)
; CHECK: %[[AND:[^ ]+]] = and <32 x i16> %[[WRREG2]], %[[WRREG1]]
; CHECK: %[[OR:[^ ]+]] = or <32 x i16> %[[WRREG2]], %[[WRREG1]]
; CHECK: %[[XOR:[^ ]+]] = xor <32 x i16> %[[WRREG2]], %[[WRREG1]]
; CHECK: {{.*}} = call <17 x i16> @llvm.genx.rdregioni.v17i16.v32i16.i16(<32 x i16> %[[AND]], i32 0, i32 17, i32 1, i16 0, i32 undef)
; CHECK: {{.*}} = call <17 x i16> @llvm.genx.rdregioni.v17i16.v32i16.i16(<32 x i16> %[[OR]], i32 0, i32 17, i32 1, i16 0, i32 undef)
; CHECK: {{.*}} = call <17 x i16> @llvm.genx.rdregioni.v17i16.v32i16.i16(<32 x i16> %[[XOR]], i32 0, i32 17, i32 1, i16 0, i32 undef)


  %5 = bitcast i32 %xor.i.i to <32 x i1>
  %call.i.i59 = zext <32 x i1> %5 to <32 x i16>
  %rdr.i.i60 = call <17 x i16> @llvm.genx.rdregioni.v17i16.v32i16.i16(<32 x i16> %call.i.i59, i32 0, i32 17, i32 1, i16 0, i32 undef)
  ret void
}
