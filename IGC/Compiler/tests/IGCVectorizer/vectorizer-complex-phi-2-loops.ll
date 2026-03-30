;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt -S  --igc-vectorizer -dce --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 < %s 2>&1 | FileCheck %s

; CHECK-LABEL: loop:
; CHECK: [[VECT_PHI:%vectorized_phi.*]] = phi <8 x float> [ <float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000>, %.lr.ph ], [ [[VECTOR:%vector.*]], %outer ]
; CHECK: br label %[[BB2:.*]]

; CHECK: [[BB2]]
; CHECK: br label %[[BB3:.*]]

; CHECK: [[BB3]]

; CHECK: phi <8 x float> [ <float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000, float 0xFFF0000000000000>, %.crit_edge1 ], [ [[VECTOR]], %._crit_edge ]
; CHECK: [[VECTOR]] = insertelement <8 x float>
; CHECK: fsub <8 x float> [[VECT_PHI]], [[VECTOR]]

; CHECK: [[VECT_BIN2:%vectorized_binary.*]] = fmul contract <8 x float> %vectorized_phi
; CHECK: [[VECT_BIN:%vectorized_binary.*]] = fmul contract <8 x float> %vectorized_phi

; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[VECT_BIN]], <8 x i16>
; CHECK: call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> [[VECT_BIN2]], <8 x i16>

; CHECK: br i1 %{{.*}}, label %{{.*}}, label %loop

source_filename = "reduced.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @phi_dpas_acc_test(ptr addrspace(1) %out, ptr addrspace(3) %slm) #0 {
entry:
  br label %.lr.ph

.lr.ph:
  br label %loop

loop:                                             ; preds = %.lr.ph, %._crit_edge
  %m0 = phi float [ 0xFFF0000000000000, %.lr.ph ], [ %mn0, %outer ]
  %m1 = phi float [ 0xFFF0000000000000, %.lr.ph ], [ %mn1, %outer ]
  %m2 = phi float [ 0xFFF0000000000000, %.lr.ph ], [ %mn2, %outer ]
  %m3 = phi float [ 0xFFF0000000000000, %.lr.ph ], [ %mn3, %outer ]
  %m4 = phi float [ 0xFFF0000000000000, %.lr.ph ], [ %mn4, %outer ]
  %m5 = phi float [ 0xFFF0000000000000, %.lr.ph ], [ %mn5, %outer ]
  %m6 = phi float [ 0xFFF0000000000000, %.lr.ph ], [ %mn6, %outer ]
  %m7 = phi float [ 0xFFF0000000000000, %.lr.ph ], [ %mn7, %outer ]
  %v0 = phi float [ 0.000000e+00, %.lr.ph ], [ %vn0, %outer ]
  %v1 = phi float [ 0.000000e+00, %.lr.ph ], [ %vn1, %outer ]
  %v2 = phi float [ 0.000000e+00, %.lr.ph ], [ %vn2, %outer ]
  %v3 = phi float [ 0.000000e+00, %.lr.ph ], [ %vn3, %outer ]
  %v4 = phi float [ 0.000000e+00, %.lr.ph ], [ %vn4, %outer ]
  %v5 = phi float [ 0.000000e+00, %.lr.ph ], [ %vn5, %outer ]
  %v6 = phi float [ 0.000000e+00, %.lr.ph ], [ %vn6, %outer ]
  %v7 = phi float [ 0.000000e+00, %.lr.ph ], [ %vn7, %outer ]
  %o0 = phi float [ 0.000000e+00, %.lr.ph ], [ %on0, %outer ]
  %o1 = phi float [ 0.000000e+00, %.lr.ph ], [ %on1, %outer ]
  %o2 = phi float [ 0.000000e+00, %.lr.ph ], [ %on2, %outer ]
  %o3 = phi float [ 0.000000e+00, %.lr.ph ], [ %on3, %outer ]
  %o4 = phi float [ 0.000000e+00, %.lr.ph ], [ %on4, %outer ]
  %o5 = phi float [ 0.000000e+00, %.lr.ph ], [ %on5, %outer ]
  %o6 = phi float [ 0.000000e+00, %.lr.ph ], [ %on6, %outer ]
  %o7 = phi float [ 0.000000e+00, %.lr.ph ], [ %on7, %outer ]
  br label %.crit_edge1

.crit_edge1:                                      ; preds = %555
  br label %._crit_edge

._crit_edge:                                      ; preds = %.crit_edge1
  %inner0 = phi float [ 0xFFF0000000000000, %.crit_edge1 ], [ %mn0, %._crit_edge ]
  %inner1 = phi float [ 0xFFF0000000000000, %.crit_edge1 ], [ %mn1, %._crit_edge ]
  %inner2 = phi float [ 0xFFF0000000000000, %.crit_edge1 ], [ %mn2, %._crit_edge ]
  %inner3 = phi float [ 0xFFF0000000000000, %.crit_edge1 ], [ %mn3, %._crit_edge ]
  %inner4 = phi float [ 0xFFF0000000000000, %.crit_edge1 ], [ %mn4, %._crit_edge ]
  %inner5 = phi float [ 0xFFF0000000000000, %.crit_edge1 ], [ %mn5, %._crit_edge ]
  %inner6 = phi float [ 0xFFF0000000000000, %.crit_edge1 ], [ %mn6, %._crit_edge ]
  %inner7 = phi float [ 0xFFF0000000000000, %.crit_edge1 ], [ %mn7, %._crit_edge ]
  %x0 = call float @llvm.maxnum.f32(float %m0, float %v0)
  %x1 = call float @llvm.maxnum.f32(float %m1, float %v1)
  %x2 = call float @llvm.maxnum.f32(float %m2, float %v2)
  %x3 = call float @llvm.maxnum.f32(float %m3, float %v3)
  %x4 = call float @llvm.maxnum.f32(float %m4, float %v4)
  %x5 = call float @llvm.maxnum.f32(float %m5, float %v5)
  %x6 = call float @llvm.maxnum.f32(float %m6, float %v6)
  %x7 = call float @llvm.maxnum.f32(float %m7, float %v7)
  %cmp0 = fcmp one float 0.000000e+00, 0.000000e+00
  %cmp1 = fcmp one float 0.000000e+00, 0.000000e+00
  %cmp2 = fcmp one float 0.000000e+00, 0.000000e+00
  %cmp3 = fcmp one float 0.000000e+00, 0.000000e+00
  %cmp4 = fcmp one float 0.000000e+00, 0.000000e+00
  %cmp5 = fcmp one float 0.000000e+00, 0.000000e+00
  %cmp6 = fcmp one float 0.000000e+00, 0.000000e+00
  %cmp7 = fcmp one float 0.000000e+00, 0.000000e+00
  %mn0 = select i1 %cmp0, float %x0, float %inner0
  %mn1 = select i1 %cmp1, float %x1, float %inner1
  %mn2 = select i1 %cmp2, float %x2, float %inner2
  %mn3 = select i1 %cmp3, float %x3, float %inner3
  %mn4 = select i1 %cmp4, float %x4, float %inner4
  %mn5 = select i1 %cmp5, float %x5, float %inner5
  %mn6 = select i1 %cmp6, float %x6, float %inner6
  %mn7 = select i1 %cmp7, float %x7, float %inner7
  %d0 = fsub float %m0, %mn0
  %d1 = fsub float %m1, %mn1
  %d2 = fsub float %m2, %mn2
  %d3 = fsub float %m3, %mn3
  %d4 = fsub float %m4, %mn4
  %d5 = fsub float %m5, %mn5
  %d6 = fsub float %m6, %mn6
  %d7 = fsub float %m7, %mn7
  %e0 = fmul contract float %d0, 0.000000e+00
  %exp0 = call float @llvm.exp2.f32(float %e0)
  %e1 = fmul contract float %d1, 0.000000e+00
  %exp1 = call float @llvm.exp2.f32(float %e1)
  %e2 = fmul contract float %d2, 0.000000e+00
  %exp2 = call float @llvm.exp2.f32(float %e2)
  %e3 = fmul contract float %d3, 0.000000e+00
  %exp3 = call float @llvm.exp2.f32(float %e3)
  %e4 = fmul contract float %d4, 0.000000e+00
  %exp4 = call float @llvm.exp2.f32(float %e4)
  %e5 = fmul contract float %d5, 0.000000e+00
  %exp5 = call float @llvm.exp2.f32(float %e5)
  %e6 = fmul contract float %d6, 0.000000e+00
  %exp6 = call float @llvm.exp2.f32(float %e6)
  %e7 = fmul contract float %d7, 0.000000e+00
  %exp7 = call float @llvm.exp2.f32(float %e7)

  %sv0 = fmul contract float %v0, %exp0
  %sv1 = fmul contract float %v1, %exp1
  %sv2 = fmul contract float %v2, %exp2
  %sv3 = fmul contract float %v3, %exp3
  %sv4 = fmul contract float %v4, %exp4
  %sv5 = fmul contract float %v5, %exp5
  %sv6 = fmul contract float %v6, %exp6
  %sv7 = fmul contract float %v7, %exp7

  %so0 = fmul contract float %o0, %exp0
  %so1 = fmul contract float %o1, %exp1
  %so2 = fmul contract float %o2, %exp2
  %so3 = fmul contract float %o3, %exp3
  %so4 = fmul contract float %o4, %exp4
  %so5 = fmul contract float %o5, %exp5
  %so6 = fmul contract float %o6, %exp6
  %so7 = fmul contract float %o7, %exp7

  %av0 = insertelement <8 x float> zeroinitializer, float %sv0, i64 0
  %av1 = insertelement <8 x float> %av0, float %sv1, i64 1
  %av2 = insertelement <8 x float> %av1, float %sv2, i64 2
  %av3 = insertelement <8 x float> %av2, float %sv3, i64 3
  %av4 = insertelement <8 x float> %av3, float %sv4, i64 4
  %av5 = insertelement <8 x float> %av4, float %sv5, i64 5
  %av6 = insertelement <8 x float> %av5, float %sv6, i64 6
  %av7 = insertelement <8 x float> %av6, float %sv7, i64 7
  %ao0 = insertelement <8 x float> zeroinitializer, float %so0, i64 0
  %ao1 = insertelement <8 x float> %ao0, float %so1, i64 1
  %ao2 = insertelement <8 x float> %ao1, float %so2, i64 2
  %ao3 = insertelement <8 x float> %ao2, float %so3, i64 3
  %ao4 = insertelement <8 x float> %ao3, float %so4, i64 4
  %ao5 = insertelement <8 x float> %ao4, float %so5, i64 5
  %ao6 = insertelement <8 x float> %ao5, float %so6, i64 6
  %ao7 = insertelement <8 x float> %ao6, float %so7, i64 7

  %kh = load <16 x half>, ptr addrspace(3) %slm, align 16
  %kv = bitcast <16 x half> %kh to <8 x i32>
  %bm = load <8 x i16>, ptr addrspace(3) %slm, align 16

  %res_v = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32( <8 x float> %av7, <8 x i16> %bm, <8 x i32> %kv, i32 12, i32 12, i32 8, i32 8, i1 false)
  %res_o = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32( <8 x float> %ao7, <8 x i16> %bm, <8 x i32> %kv, i32 12, i32 12, i32 8, i32 8, i1 false)

  %vn0 = extractelement <8 x float> %res_v, i64 0
  %vn1 = extractelement <8 x float> %res_v, i64 1
  %vn2 = extractelement <8 x float> %res_v, i64 2
  %vn3 = extractelement <8 x float> %res_v, i64 3
  %vn4 = extractelement <8 x float> %res_v, i64 4
  %vn5 = extractelement <8 x float> %res_v, i64 5
  %vn6 = extractelement <8 x float> %res_v, i64 6
  %vn7 = extractelement <8 x float> %res_v, i64 7
  %on0 = extractelement <8 x float> %res_o, i64 0
  %on1 = extractelement <8 x float> %res_o, i64 1
  %on2 = extractelement <8 x float> %res_o, i64 2
  %on3 = extractelement <8 x float> %res_o, i64 3
  %on4 = extractelement <8 x float> %res_o, i64 4
  %on5 = extractelement <8 x float> %res_o, i64 5
  %on6 = extractelement <8 x float> %res_o, i64 6
  %on7 = extractelement <8 x float> %res_o, i64 7

  %loop_done = icmp eq i64 0, 0
  br i1 %loop_done, label %._crit_edge, label %outer

outer:
  %loop_inner_done = icmp eq i64 0, 0
  br i1 %loop_inner_done, label %exit, label %loop

exit:
  store float %vn0, ptr addrspace(1) %out, align 4
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i1, i32) #1
declare float @llvm.maxnum.f32(float, float) #2
declare float @llvm.exp2.f32(float) #2

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}
!0 = !{ptr @phi_dpas_acc_test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
