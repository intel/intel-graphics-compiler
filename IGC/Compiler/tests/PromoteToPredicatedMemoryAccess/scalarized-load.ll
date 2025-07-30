;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers --enable-debugify --igc-promote-to-predicated-memory-access --platformbmg -S < %s 2>&1 | FileCheck %s

; Debug-info related check
; 11 warnings are related to 11 deleted phi instructions
; CHECK-COUNT-11: WARNING
; CHECK: CheckModuleDebugify: PASS

; Test basic logic
; CHECK-LABEL: @test1(
define <4 x float> @test1(<4 x float> addrspace(1)* %src, i1 %pred) {
entry:
; CHECK: br label %st
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = call <4 x float> @llvm.genx.GenISA.PredicatedLoad.v4f32.p1v4f32.v4f32(<4 x float> addrspace(1)* %src, i64 16, i1 %pred, <4 x float> zeroinitializer)
  %load = load <4 x float>, <4 x float> addrspace(1)* %src, align 16
  %.scalar = extractelement <4 x float> %load, i64 0
  %.scalar15 = extractelement <4 x float> %load, i64 1
  %.scalar16 = extractelement <4 x float> %load, i64 2
  %.scalar17 = extractelement <4 x float> %load, i64 3
  br label %exit

exit:
; CHECK-NOT: phi
; CHECK: %res0 = insertelement <4 x float> undef, float %.scalar, i64 0
; CHECK: %res1 = insertelement <4 x float> %res0, float %.scalar15, i64 1
; CHECK: %res2 = insertelement <4 x float> %res1, float %.scalar16, i64 2
; CHECK: %res3 = insertelement <4 x float> %res2, float %.scalar17, i64 3
  %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
  %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
  %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
  %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %res0 = insertelement <4 x float> undef, float %bc311, i64 0
  %res1 = insertelement <4 x float> %res0, float %bc312, i64 1
  %res2 = insertelement <4 x float> %res1, float %bc313, i64 2
  %res3 = insertelement <4 x float> %res2, float %bc314, i64 3
  ret <4 x float> %res3
}

; Test non-constant merge values
; CHECK-LABEL: @test2(
define <4 x float> @test2(<4 x float> addrspace(1)* %src, i1 %pred, float %d1, float %d2, float %d3, float %d4) {
entry:
; CHECK: br label %st
  br i1 %pred, label %st, label %exit

st:
; CHECK: [[TMP0:%[0-9]+]] = insertelement <4 x float> poison, float %d1, i64 0
; CHECK: [[TMP1:%[0-9]+]] = insertelement <4 x float> [[TMP0]], float %d3, i64 1
; CHECK: [[TMP2:%[0-9]+]] = insertelement <4 x float> [[TMP1]], float %d2, i64 2
; CHECK: [[TMP3:%[0-9]+]] = insertelement <4 x float> [[TMP2]], float %d4, i64 3
; CHECK: %load = call <4 x float> @llvm.genx.GenISA.PredicatedLoad.v4f32.p1v4f32.v4f32(<4 x float> addrspace(1)* %src, i64 16, i1 %pred, <4 x float> [[TMP3]])
  %load = load <4 x float>, <4 x float> addrspace(1)* %src, align 16
  %.scalar = extractelement <4 x float> %load, i64 0
  %.scalar15 = extractelement <4 x float> %load, i64 1
  %.scalar16 = extractelement <4 x float> %load, i64 2
  %.scalar17 = extractelement <4 x float> %load, i64 3
  br label %exit

exit:
; CHECK-NOT: phi
; CHECK: %res0 = insertelement <4 x float> undef, float %.scalar, i64 0
; CHECK: %res1 = insertelement <4 x float> %res0, float %.scalar15, i64 1
; CHECK: %res2 = insertelement <4 x float> %res1, float %.scalar16, i64 2
; CHECK: %res3 = insertelement <4 x float> %res2, float %.scalar17, i64 3
  %bc311 = phi float [ %.scalar, %st ], [ %d1, %entry ]
  %bc312 = phi float [ %.scalar15, %st ], [ %d3, %entry ]
  %bc313 = phi float [ %.scalar16, %st ], [ %d2, %entry ]
  %bc314 = phi float [ %.scalar17, %st ], [ %d4, %entry ]
  %res0 = insertelement <4 x float> undef, float %bc311, i64 0
  %res1 = insertelement <4 x float> %res0, float %bc312, i64 1
  %res2 = insertelement <4 x float> %res1, float %bc313, i64 2
  %res3 = insertelement <4 x float> %res2, float %bc314, i64 3
  ret <4 x float> %res3
}

; skip case with non-constant indexes
; CHECK-LABEL: @test3(
define <4 x float> @test3(<4 x float> addrspace(1)* %src, i1 %pred, i64 %id0, i64 %id1, i64 %id2, i64 %id3) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = load <4 x float>, <4 x float> addrspace(1)* %src, align 16
  %load = load <4 x float>, <4 x float> addrspace(1)* %src, align 16
  %.scalar = extractelement <4 x float> %load, i64 %id0
  %.scalar15 = extractelement <4 x float> %load, i64 %id1
  %.scalar16 = extractelement <4 x float> %load, i64 %id2
  %.scalar17 = extractelement <4 x float> %load, i64 %id3
  br label %exit

exit:
; CHECK: %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
  %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
  %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
  %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %res0 = insertelement <4 x float> undef, float %bc311, i64 0
  %res1 = insertelement <4 x float> %res0, float %bc312, i64 1
  %res2 = insertelement <4 x float> %res1, float %bc313, i64 2
  %res3 = insertelement <4 x float> %res2, float %bc314, i64 3
  ret <4 x float> %res3
}

; Test load is not in the same BB
; CHECK-LABEL: @test4(
define <4 x float> @test4(<4 x float> addrspace(1)* %src, i1 %pred) {
entry:
; CHECK: %load = load <4 x float>, <4 x float> addrspace(1)* %src, align 16
; CHECK: br i1 %pred, label %st, label %exit
  %load = load <4 x float>, <4 x float> addrspace(1)* %src, align 16
  br i1 %pred, label %st, label %exit

st:
  %.scalar = extractelement <4 x float> %load, i64 0
  %.scalar15 = extractelement <4 x float> %load, i64 1
  %.scalar16 = extractelement <4 x float> %load, i64 2
  %.scalar17 = extractelement <4 x float> %load, i64 3
  br label %exit

exit:
; CHECK: %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
  %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
  %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
  %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %res0 = insertelement <4 x float> undef, float %bc311, i64 0
  %res1 = insertelement <4 x float> %res0, float %bc312, i64 1
  %res2 = insertelement <4 x float> %res1, float %bc313, i64 2
  %res3 = insertelement <4 x float> %res2, float %bc314, i64 3
  ret <4 x float> %res3
}

; not all values are used
; CHECK-LABEL: @test5(
define <4 x float> @test5(<4 x float> addrspace(1)* %src, i1 %pred, float %d1, float %d2, float %d3) {
entry:
; CHECK: br label %st
  br i1 %pred, label %st, label %exit

st:
; CHECK: [[TMP0:%[0-9]+]] = insertelement <4 x float> poison, float %d1, i64 0
; CHECK: [[TMP1:%[0-9]+]] = insertelement <4 x float> [[TMP0]], float %d2, i64 1
; CHECK: [[TMP2:%[0-9]+]] = insertelement <4 x float> [[TMP1]], float 0.000000e+00, i64 2
; CHECK: [[TMP3:%[0-9]+]] = insertelement <4 x float> [[TMP2]], float %d3, i64 3
; CHECK: %load = call <4 x float> @llvm.genx.GenISA.PredicatedLoad.v4f32.p1v4f32.v4f32(<4 x float> addrspace(1)* %src, i64 16, i1 %pred, <4 x float> [[TMP3]])
  %load = load <4 x float>, <4 x float> addrspace(1)* %src, align 16
  %.scalar = extractelement <4 x float> %load, i64 0
  %.scalar15 = extractelement <4 x float> %load, i64 1
  %.scalar16 = extractelement <4 x float> %load, i64 2
  %.scalar17 = extractelement <4 x float> %load, i64 3
  br label %exit

exit:
; CHECK-NOT: phi
; CHECK: %res0 = insertelement <4 x float> undef, float %.scalar, i64 0
; CHECK: %res1 = insertelement <4 x float> %res0, float %.scalar15, i64 1
; CHECK: %res3 = insertelement <4 x float> %res1, float %.scalar17, i64 3
  %bc311 = phi float [ %.scalar, %st ], [ %d1, %entry ]
  %bc312 = phi float [ %.scalar15, %st ], [ %d2, %entry ]
  %bc314 = phi float [ %.scalar17, %st ], [ %d3, %entry ]
  %res0 = insertelement <4 x float> undef, float %bc311, i64 0
  %res1 = insertelement <4 x float> %res0, float %bc312, i64 1
  %res3 = insertelement <4 x float> %res1, float %bc314, i64 3
  ret <4 x float> %res3
}

; Test extractelement is not from load
; CHECK-LABEL: @test6(
define <4 x float> @test6(<4 x float> addrspace(1)* %src, i1 %pred, <4 x float> %data) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
  %.scalar = extractelement <4 x float> %data, i64 0
  %.scalar15 = extractelement <4 x float> %data, i64 1
  %.scalar16 = extractelement <4 x float> %data, i64 2
  %.scalar17 = extractelement <4 x float> %data, i64 3
  br label %exit

exit:
; CHECK: %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
  %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
  %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
  %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %res0 = insertelement <4 x float> undef, float %bc311, i64 0
  %res1 = insertelement <4 x float> %res0, float %bc312, i64 1
  %res2 = insertelement <4 x float> %res1, float %bc313, i64 2
  %res3 = insertelement <4 x float> %res2, float %bc314, i64 3
  ret <4 x float> %res3
}

; Test load can not be converted to predicated load
; CHECK-LABEL: @test7(
define <4 x float> @test7(<4 x float> addrspace(1)* %src, i1 %pred) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = load volatile <4 x float>, <4 x float> addrspace(1)* %src, align 16
  %load = load volatile <4 x float>, <4 x float> addrspace(1)* %src, align 16
  %.scalar = extractelement <4 x float> %load, i64 0
  %.scalar15 = extractelement <4 x float> %load, i64 1
  %.scalar16 = extractelement <4 x float> %load, i64 2
  %.scalar17 = extractelement <4 x float> %load, i64 3
  br label %exit

exit:
; CHECK: %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
  %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
  %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
  %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %res0 = insertelement <4 x float> undef, float %bc311, i64 0
  %res1 = insertelement <4 x float> %res0, float %bc312, i64 1
  %res2 = insertelement <4 x float> %res1, float %bc313, i64 2
  %res3 = insertelement <4 x float> %res2, float %bc314, i64 3
  ret <4 x float> %res3
}

; Test index is outside of vector size
; CHECK-LABEL: @test8(
define <4 x float> @test8(<4 x float> addrspace(1)* %src, i1 %pred) {
entry:
; CHECK: br i1 %pred, label %st, label %exit
  br i1 %pred, label %st, label %exit

st:
; CHECK: %load = load <4 x float>, <4 x float> addrspace(1)* %src, align 16
  %load = load <4 x float>, <4 x float> addrspace(1)* %src, align 16
  %.scalar = extractelement <4 x float> %load, i64 0
  %.scalar15 = extractelement <4 x float> %load, i64 1
  %.scalar16 = extractelement <4 x float> %load, i64 2
  %.scalar17 = extractelement <4 x float> %load, i64 4
  br label %exit

exit:
; CHECK: %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
; CHECK: %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %bc311 = phi float [ %.scalar, %st ], [ 0.000000e+00, %entry ]
  %bc312 = phi float [ %.scalar15, %st ], [ 0.000000e+00, %entry ]
  %bc313 = phi float [ %.scalar16, %st ], [ 0.000000e+00, %entry ]
  %bc314 = phi float [ %.scalar17, %st ], [ 0.000000e+00, %entry ]
  %res0 = insertelement <4 x float> undef, float %bc311, i64 0
  %res1 = insertelement <4 x float> %res0, float %bc312, i64 1
  %res2 = insertelement <4 x float> %res1, float %bc313, i64 2
  %res3 = insertelement <4 x float> %res2, float %bc314, i64 3
  ret <4 x float> %res3
}

; CHECK: warning: Index 4 is >= vector size 4
