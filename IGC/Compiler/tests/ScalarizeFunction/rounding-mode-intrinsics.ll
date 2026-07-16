;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-scalarize -S < %s | FileCheck %s
; ------------------------------------------------
; ScalarizeFunction
; ------------------------------------------------
;
; Tests scalarization of vector floating-point rounding-mode GenISA intrinsics.
; The vector float operands are scalarized element-wise while the scalar i32
; rounding-mode operand is forwarded to every scalar call unchanged.

declare <2 x float> @llvm.genx.GenISA.IEEE.Divide.rm.v2f32(<2 x float>, <2 x float>, i32)
declare <2 x float> @llvm.genx.GenISA.IEEE.Sqrt.rm.v2f32(<2 x float>, i32)
declare <2 x half> @llvm.genx.GenISA.ftof.rte.v2f16.v2f32(<2 x float>)
declare half @llvm.genx.GenISA.ftof.rte.f16.f32(float)
declare <2 x half> @llvm.experimental.constrained.fdiv.v2f16(<2 x half>, <2 x half>, metadata, metadata)
declare <2 x half> @llvm.experimental.constrained.sqrt.v2f16(<2 x half>, metadata, metadata)

define spir_kernel void @test_div_rm(<2 x float> %a, <2 x float> %b) {
; CHECK-LABEL: @test_div_rm(
; CHECK: call float @llvm.genx.GenISA.IEEE.Divide.rm.f32(float %{{.*}}, float %{{.*}}, i32 0)
; CHECK: call float @llvm.genx.GenISA.IEEE.Divide.rm.f32(float %{{.*}}, float %{{.*}}, i32 0)
; CHECK-NOT: call <2 x float> @llvm.genx.GenISA.IEEE.Divide.rm.v2f32
  %r = call <2 x float> @llvm.genx.GenISA.IEEE.Divide.rm.v2f32(<2 x float> %a, <2 x float> %b, i32 0)
  ret void
}

define spir_kernel void @test_sqrt_rm(<2 x float> %a) {
; CHECK-LABEL: @test_sqrt_rm(
; CHECK: call float @llvm.genx.GenISA.IEEE.Sqrt.rm.f32(float %{{.*}}, i32 0)
; CHECK: call float @llvm.genx.GenISA.IEEE.Sqrt.rm.f32(float %{{.*}}, i32 0)
; CHECK-NOT: call <2 x float> @llvm.genx.GenISA.IEEE.Sqrt.rm.v2f32
  %r = call <2 x float> @llvm.genx.GenISA.IEEE.Sqrt.rm.v2f32(<2 x float> %a, i32 0)
  ret void
}

define spir_kernel void @test_ftof(<2 x float> %a) {
; CHECK-LABEL: @test_ftof(
; CHECK: call half @llvm.genx.GenISA.ftof.rte.f16.f32(float %{{.*}})
; CHECK: call half @llvm.genx.GenISA.ftof.rte.f16.f32(float %{{.*}})
; CHECK-NOT: call <2 x half> @llvm.genx.GenISA.ftof.rte.v2f16.v2f32
  %r = call <2 x half> @llvm.genx.GenISA.ftof.rte.v2f16.v2f32(<2 x float> %a)
  ret void
}

define spir_kernel void @test_constrained_div(<2 x half> %a, <2 x half> %b) {
; CHECK-LABEL: @test_constrained_div(
; CHECK: call half @llvm.experimental.constrained.fdiv.f16(half %{{.*}}, half %{{.*}}, metadata !"round.towardzero", metadata !"fpexcept.ignore")
; CHECK: call half @llvm.experimental.constrained.fdiv.f16(half %{{.*}}, half %{{.*}}, metadata !"round.towardzero", metadata !"fpexcept.ignore")
; CHECK-NOT: call <2 x half> @llvm.experimental.constrained.fdiv.v2f16
  %r = call <2 x half> @llvm.experimental.constrained.fdiv.v2f16(<2 x half> %a, <2 x half> %b, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  ret void
}

define spir_kernel void @test_constrained_sqrt(<2 x half> %a) {
; CHECK-LABEL: @test_constrained_sqrt(
; CHECK: call half @llvm.experimental.constrained.sqrt.f16(half %{{.*}}, metadata !"round.towardzero", metadata !"fpexcept.ignore")
; CHECK: call half @llvm.experimental.constrained.sqrt.f16(half %{{.*}}, metadata !"round.towardzero", metadata !"fpexcept.ignore")
; CHECK-NOT: call <2 x half> @llvm.experimental.constrained.sqrt.v2f16
  %r = call <2 x half> @llvm.experimental.constrained.sqrt.v2f16(<2 x half> %a, metadata !"round.towardzero", metadata !"fpexcept.ignore")
  ret void
}

; sanity - scalar intrinsics are left untouched

define spir_kernel void @test_scalar_sanity(float %a) {
; CHECK-LABEL: @test_scalar_sanity(
; CHECK: call half @llvm.genx.GenISA.ftof.rte.f16.f32(float %a)
  %r = call half @llvm.genx.GenISA.ftof.rte.f16.f32(float %a)
  ret void
}
