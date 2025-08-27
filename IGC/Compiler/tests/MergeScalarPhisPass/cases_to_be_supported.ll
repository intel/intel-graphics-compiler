;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -o - -merge-scalar-phis | FileCheck %s

; These patterns are not yet supported by the pass but should be in the future.
; XFAIL: *

declare void @use_4scalars(float, float, float, float)
declare <4 x float> @update_vec4float(<4 x float>)

; Test phi vectorization in case incoming values ​​are mixed.

; CHECK-LABEL: define void @f0
; CHECK: entry:
; CHECK-NOT: %eei_1_1 = extractelement <4 x float> %inc_vec, i64 0
; CHECK-NOT: %eei_1_2 = extractelement <4 x float> %inc_vec, i64 1
; CHECK-NOT: %eei_1_3 = extractelement <4 x float> %inc_vec, i64 2
; CHECK-NOT: %eei_1_4 = extractelement <4 x float> %inc_vec, i64 3
; CHECK: br label %BB1
; CHECK: [[MERGED_PHI:%.*]] = phi <4 x float>
; CHECK: [[NEW_EEI1:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 0
; CHECK: [[NEW_EEI2:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 1
; CHECK: [[NEW_EEI3:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 2
; CHECK: [[NEW_EEI4:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 3
; CHECK: call void @use_4scalars(float %phi1, float %phi2, float %phi3, float %phi4)
define void @f0(i1 %cond1, i1 %cond2, <4 x float> %inc_vec) {
entry:
  %eei_1_1 = extractelement <4 x float> %inc_vec, i64 0
  %eei_1_2 = extractelement <4 x float> %inc_vec, i64 1
  %eei_1_3 = extractelement <4 x float> %inc_vec, i64 2
  %eei_1_4 = extractelement <4 x float> %inc_vec, i64 3
  br label %BB1
BB1:
  %phi1 = phi float [ %eei_1_1, %BB1 ], [ 0.0, %entry ]
  %phi2 = phi float [ 0.0, %entry ], [ %eei_1_2, %BB1 ]
  %phi3 = phi float [ %eei_1_3, %BB1 ], [ 0.0, %entry ]
  %phi4 = phi float [ %eei_1_4, %BB1 ], [ 0.0, %entry ]
  call void @use_4scalars(float %phi1, float %phi2, float %phi3, float %phi4)
  br i1 %cond1, label %BB1, label %BBEXIT
BBEXIT:
  ret void
}

!igc.functions = !{!0}
!0 = !{void (i1, i1, <4 x float>)* @f0, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}