;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --igc-vectorizer -S -dce --platformbmg < %s 2>&1 | FileCheck %s

; A fully reversed extract swizzle is not naive, so handleExtractElement cannot
; substitute the source vector.  It must instead build an "extract interface" --
; a fresh insertelement chain that gathers the extracts in slice order -- and
; feed that to the vectorized phi.  Lane N of the interface must hold the
; extract that lane N of the scalar group held.

; CHECK-LABEL: @quux(
; CHECK:         %vectorized_phi = phi <8 x float> [ zeroinitializer, {{%.*}} ], [ [[IFACE:%[^,]+]], {{%.*}} ]
; CHECK:         [[DPAS:%[a-zA-Z0-9_]+]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas
; CHECK-NEXT:    [[E0:%[a-zA-Z0-9_]+]] = extractelement <8 x float> [[DPAS]], i64 7
; CHECK-NEXT:    [[E1:%[a-zA-Z0-9_]+]] = extractelement <8 x float> [[DPAS]], i64 6
; CHECK:         [[E7:%[a-zA-Z0-9_]+]] = extractelement <8 x float> [[DPAS]], i64 0
; CHECK-NEXT:    [[I0:%[a-zA-Z0-9_]+]] = insertelement <8 x float> undef, float [[E0]], i32 0
; CHECK-NEXT:    [[I1:%[a-zA-Z0-9_]+]] = insertelement <8 x float> [[I0]], float [[E1]], i32 1
; CHECK:         [[IFACE]] = insertelement <8 x float> {{%[a-zA-Z0-9_]+}}, float [[E7]], i32 7
target triple = "spir64-unknown-unknown"

define spir_kernel void @quux() {
entry:
  br label %loop

loop:
  %p0 = phi float [ 0.000000e+00, %entry ], [ %e0, %loop ]
  %p1 = phi float [ 0.000000e+00, %entry ], [ %e1, %loop ]
  %p2 = phi float [ 0.000000e+00, %entry ], [ %e2, %loop ]
  %p3 = phi float [ 0.000000e+00, %entry ], [ %e3, %loop ]
  %p4 = phi float [ 0.000000e+00, %entry ], [ %e4, %loop ]
  %p5 = phi float [ 0.000000e+00, %entry ], [ %e5, %loop ]
  %p6 = phi float [ 0.000000e+00, %entry ], [ %e6, %loop ]
  %p7 = phi float [ 0.000000e+00, %entry ], [ %e7, %loop ]
  %x0 = call float @llvm.exp2.f32(float 1.000000e+00)
  %x1 = call float @llvm.exp2.f32(float 2.000000e+00)
  %x2 = call float @llvm.exp2.f32(float 3.000000e+00)
  %x3 = call float @llvm.exp2.f32(float 4.000000e+00)
  %x4 = call float @llvm.exp2.f32(float 5.000000e+00)
  %x5 = call float @llvm.exp2.f32(float 6.000000e+00)
  %x6 = call float @llvm.exp2.f32(float 7.000000e+00)
  %x7 = call float @llvm.exp2.f32(float 8.000000e+00)
  %m0 = fmul fast float %x0, %p0
  %m1 = fmul fast float %x1, %p1
  %m2 = fmul fast float %x2, %p2
  %m3 = fmul fast float %x3, %p3
  %m4 = fmul fast float %x4, %p4
  %m5 = fmul fast float %x5, %p5
  %m6 = fmul fast float %x6, %p6
  %m7 = fmul fast float %x7, %p7
  %v0 = insertelement <8 x float> zeroinitializer, float %m0, i64 0
  %v1 = insertelement <8 x float> %v0, float %m1, i64 1
  %v2 = insertelement <8 x float> %v1, float %m2, i64 2
  %v3 = insertelement <8 x float> %v2, float %m3, i64 3
  %v4 = insertelement <8 x float> %v3, float %m4, i64 4
  %v5 = insertelement <8 x float> %v4, float %m5, i64 5
  %v6 = insertelement <8 x float> %v5, float %m6, i64 6
  %v7 = insertelement <8 x float> %v6, float %m7, i64 7
  %dpasA = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %v7, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %e0 = extractelement <8 x float> %dpasA, i64 7
  %e1 = extractelement <8 x float> %dpasA, i64 6
  %e2 = extractelement <8 x float> %dpasA, i64 5
  %e3 = extractelement <8 x float> %dpasA, i64 4
  %e4 = extractelement <8 x float> %dpasA, i64 3
  %e5 = extractelement <8 x float> %dpasA, i64 2
  %e6 = extractelement <8 x float> %dpasA, i64 1
  %e7 = extractelement <8 x float> %dpasA, i64 0
  br label %loop
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #0
declare float @llvm.exp2.f32(float) #1

attributes #0 = { convergent nounwind readnone willreturn }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}
!0 = !{void ()* @quux, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void ()* @quux}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
