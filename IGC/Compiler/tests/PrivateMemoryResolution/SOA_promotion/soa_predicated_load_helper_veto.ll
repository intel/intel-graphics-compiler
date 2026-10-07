;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; A scalar GenISA_PredicatedLoad is only rewritten by the new-algo TransposePrivMem helper.
; SOALayoutChecker must veto the alloca for every other helper, otherwise the predicated load is left on
; its untransposed address while the plain loads/stores of the same alloca are transposed.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-priv-mem-to-reg \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnablePrivMemNewSOAForScalarArrays=1,EnableAggressiveSOAPromotion=0 \
; RUN:   -S %s | FileCheck %s --check-prefix=GRF
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=0,EnableSOAFallbackToOldAlgorithm=1,EnableAggressiveSOAPromotion=0 \
; RUN:   -S %s | FileCheck %s --check-prefixes=AOS,AOS-SCALAR
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnablePrivMemNewSOAForScalarArrays=1,EnableSOAFallbackToOldAlgorithm=1,EnableAggressiveSOAPromotion=0 \
; RUN:   -S %s | FileCheck %s --check-prefixes=AOS,NEW

target datalayout = "e-p:32:32:32-i64:64-n8:16:32:64"

; GRF-LABEL: @scalar_access(
; GRF-NOT:     alloca <8 x float>
; GRF:         %a = alloca [8 x float]
; GRF:         call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %p, i64 4, i1 %c, float 5.000000e+00)
;
; AOS-SCALAR-LABEL: @scalar_access(
; AOS-SCALAR:       [[LANE:%.*]] = zext i16 {{%.*}} to i32
; AOS-SCALAR:       mul i32 [[LANE]], 32
; AOS-SCALAR:       [[P:%.*]] = getelementptr [8 x float], ptr {{%.*}}, i32 0, i32 3
; AOS-SCALAR:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr [[P]], i64 4, i1 %c, float 5.000000e+00)
;
; NEW-LABEL: @scalar_access(
; NEW:       [[LANE:%.*]] = zext i16 {{%.*}} to i32
; NEW:       mul i32 [[LANE]], 4
; NEW-NOT:   getelementptr [8 x float]
; NEW:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr {{%.*}}, i64 4, i1 %c, float 5.000000e+00)
define spir_kernel void @scalar_access(i32 %i, i1 %c, ptr addrspace(1) %out) {
entry:
  %a = alloca [8 x float], align 4
  %s = getelementptr [8 x float], ptr %a, i32 0, i32 %i
  store float 1.0, ptr %s, align 4
  %p = getelementptr [8 x float], ptr %a, i32 0, i32 3
  %v = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %p, i64 4, i1 %c, float 5.0)
  store float %v, ptr addrspace(1) %out, align 4
  ret void
}

; The <4 x float> store makes the new algo decline, so the legacy SoA would be
; picked by default; the predicated load has to veto it.
;
; AOS-LABEL: @wide_access(
; AOS:       [[LANE:%.*]] = zext i16 {{%.*}} to i32
; AOS:       mul i32 [[LANE]], 32
; AOS:       store <4 x float> zeroinitializer, ptr
; AOS:       [[P:%.*]] = getelementptr [8 x float], ptr {{%.*}}, i32 0, i32 3
; AOS:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr [[P]], i64 4, i1 %c, float 5.000000e+00)
define spir_kernel void @wide_access(i32 %i, i1 %c, ptr addrspace(1) %out) {
entry:
  %a = alloca [8 x float], align 4
  %s = getelementptr [8 x float], ptr %a, i32 0, i32 %i
  store float 1.0, ptr %s, align 4
  store <4 x float> zeroinitializer, ptr %a, align 4
  %p = getelementptr [8 x float], ptr %a, i32 0, i32 3
  %v = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr %p, i64 4, i1 %c, float 5.0)
  store float %v, ptr addrspace(1) %out, align 4
  ret void
}

declare float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr, i64, i1, float)

!igc.functions = !{!0, !3}
!0 = !{ptr @scalar_access, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @wide_access, !1}
