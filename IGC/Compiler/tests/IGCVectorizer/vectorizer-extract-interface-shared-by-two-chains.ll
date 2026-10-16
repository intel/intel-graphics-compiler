;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --igc-vectorizer -S -dce --platformbmg < %s 2>&1 | FileCheck %s

; Two independent vectorization chains (%p... and %q...) are fed by the *same*
; non-naive extract group %e0..%e7.  handleExtractElement does not RAUW the
; extracts, so the second chain re-discovers the identical group.
;
; checkPrevVectorization finds the interface the first chain already built and
; reports it in PrevVectorization, since it holds exactly this slice in this
; order.  handleExtractElement reuses it, so both chains consume the single
; interface and no second, byte-identical one is materialised.

; CHECK-LABEL: @quux(
; CHECK:         phi <8 x float> [ zeroinitializer, {{%.*}} ], [ [[IFACE:%[^,]+]], {{%.*}} ]
; CHECK:         phi <8 x float> [ zeroinitializer, {{%.*}} ], [ [[IFACE]], {{%.*}} ]
; CHECK:         [[A:%[a-zA-Z0-9_]+]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas
; CHECK:         [[B:%[a-zA-Z0-9_]+]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas
; CHECK-NEXT:    [[E0:%[a-zA-Z0-9_]+]] = extractelement <8 x float> [[A]], i64 1
; CHECK-NEXT:    [[E1:%[a-zA-Z0-9_]+]] = extractelement <8 x float> [[A]], i64 0
; CHECK:         [[E7:%[a-zA-Z0-9_]+]] = extractelement <8 x float> [[B]], i64 6
; the only interface
; CHECK-NEXT:    [[P0:%[a-zA-Z0-9_]+]] = insertelement <8 x float> undef, float [[E0]], i32 0
; CHECK-NEXT:    [[P1:%[a-zA-Z0-9_]+]] = insertelement <8 x float> [[P0]], float [[E1]], i32 1
; CHECK:         [[IFACE]] = insertelement <8 x float> {{%[a-zA-Z0-9_]+}}, float [[E7]], i32 7
; CHECK-NOT:     insertelement
; CHECK:         br label %loop

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
  %q0 = phi float [ 0.000000e+00, %entry ], [ %e0, %loop ]
  %q1 = phi float [ 0.000000e+00, %entry ], [ %e1, %loop ]
  %q2 = phi float [ 0.000000e+00, %entry ], [ %e2, %loop ]
  %q3 = phi float [ 0.000000e+00, %entry ], [ %e3, %loop ]
  %q4 = phi float [ 0.000000e+00, %entry ], [ %e4, %loop ]
  %q5 = phi float [ 0.000000e+00, %entry ], [ %e5, %loop ]
  %q6 = phi float [ 0.000000e+00, %entry ], [ %e6, %loop ]
  %q7 = phi float [ 0.000000e+00, %entry ], [ %e7, %loop ]
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
  %n0 = fmul fast float %x0, %q0
  %n1 = fmul fast float %x1, %q1
  %n2 = fmul fast float %x2, %q2
  %n3 = fmul fast float %x3, %q3
  %n4 = fmul fast float %x4, %q4
  %n5 = fmul fast float %x5, %q5
  %n6 = fmul fast float %x6, %q6
  %n7 = fmul fast float %x7, %q7
  %a0 = insertelement <8 x float> zeroinitializer, float %m0, i64 0
  %a1 = insertelement <8 x float> %a0, float %m1, i64 1
  %a2 = insertelement <8 x float> %a1, float %m2, i64 2
  %a3 = insertelement <8 x float> %a2, float %m3, i64 3
  %a4 = insertelement <8 x float> %a3, float %m4, i64 4
  %a5 = insertelement <8 x float> %a4, float %m5, i64 5
  %a6 = insertelement <8 x float> %a5, float %m6, i64 6
  %a7 = insertelement <8 x float> %a6, float %m7, i64 7
  %b0 = insertelement <8 x float> zeroinitializer, float %n0, i64 0
  %b1 = insertelement <8 x float> %b0, float %n1, i64 1
  %b2 = insertelement <8 x float> %b1, float %n2, i64 2
  %b3 = insertelement <8 x float> %b2, float %n3, i64 3
  %b4 = insertelement <8 x float> %b3, float %n4, i64 4
  %b5 = insertelement <8 x float> %b4, float %n5, i64 5
  %b6 = insertelement <8 x float> %b5, float %n6, i64 6
  %b7 = insertelement <8 x float> %b6, float %n7, i64 7
  %dA = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %a7, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dB = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %b7, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %e0 = extractelement <8 x float> %dA, i64 1
  %e1 = extractelement <8 x float> %dA, i64 0
  %e2 = extractelement <8 x float> %dA, i64 3
  %e3 = extractelement <8 x float> %dA, i64 2
  %e4 = extractelement <8 x float> %dB, i64 5
  %e5 = extractelement <8 x float> %dB, i64 4
  %e6 = extractelement <8 x float> %dB, i64 7
  %e7 = extractelement <8 x float> %dB, i64 6
  br label %loop
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1
declare <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v8i16.v8i32(<4 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1
declare float @llvm.exp2.f32(float) #2
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }
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
