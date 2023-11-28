;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=109 %s -S -o - --platformmtl --igc-precompiled-import | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport
;
; ForceEmuKind=109 means EMU_I64DIVREM | EMU_DP_DIV_SQRT | EMU_SP_DIV | EMU_I32DIVREM_SP | EMU_FP64_FP16_CONV
;
; This test verifies if GenISA.fma intrinsic names clash doesn't occur when `sdiv i64` and `fdiv double`
; are simultaniously emulated in the same LLVM module. Sdiv's implementation uses fma.rtz.f32, while
; fdiv's implementation uses fma.rtz.f64. If IGC emulation builtins didn't add type mangling to GenISA_fma_rtz,
; we would end up with incorrect bitcast instruction that would result in using float fma.rtz in places where
; double version is expected.
;
; ------------------------------------------------

; CHECK-NOT: call float bitcast (double (double, double, double)* @GenISA_fma_rtz to float (float, float, float)*)(float {{.*}}, float {{.*}}, float {{.*}})

; CHECK-LABEL: @__igcbuiltin_dp_div_nomadm_ieee
; CHECK: call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double {{.*}}, double {{.*}}, double {{.*}})

; CHECK-LABEL: @__igcbuiltin_s64_sdiv_sp
; CHECK: call float @llvm.genx.GenISA.fma.rtz.f32.f32.f32.f32(float {{.*}}, float {{.*}}, float {{.*}})

define void @kernel(i64 addrspace(1)* %outA, double addrspace(1)* %outB, i64 %ix, i64 %iy, double %dx, double %dy) {
entry:
  %iresult = sdiv i64 %ix, %iy
  store i64 %iresult, i64 addrspace(1)* %outA, align 8
  %dresult = fdiv double %dx, %dy
  store double %dresult, double addrspace(1)* %outB, align 8
  ret void
}
