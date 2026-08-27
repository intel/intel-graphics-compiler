;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -o - -merge-scalar-phis | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"


declare <4 x float> @get_vec4float()
declare <2 x float> @get_vec2float()
declare <4 x float> @update_vec4float(<4 x float>)
declare <2 x float> @update_vec2float(<2 x float>)
declare void @use_vec4float(<4 x float>)
declare void @use_4scalars(float, float, float, float)
declare void @use_2scalars(float, float, float, float)
declare void @use_float(float)

; Basic example when MergeScalarPhisPass should be applied.

; CHECK-LABEL: define void @f0
;
; CHECK-NOT: phi float
; ...
; CHECK: BB1:
; CHECK: %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
; ...
; CHECK: BB3:
; CHECK: %vec2 = call <4 x float> @get_vec4float()
; ...
; CHECK: BB4:
; CHECK: [[MERGED_PHI:%.*]] = phi <4 x float> [ %vec1, %BB2 ], [ %vec2, %BB3 ]
; CHECK: [[NEW_EEI1:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 0
; CHECK: [[NEW_EEI2:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 1
; CHECK: [[NEW_EEI3:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 2
; CHECK: [[NEW_EEI4:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 3
; CHECK: call void @use_4scalars(float [[NEW_EEI1]], float [[NEW_EEI2]], float [[NEW_EEI3]], float [[NEW_EEI4]])
; CHECK: ret void

define void @f0(i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %BB1, label %BB3
BB1:
  %inc_vec = phi <4 x float> [ zeroinitializer, %entry ], [ %vec1, %BB1 ]
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  br i1 %cond2, label %BB1, label %BB2
BB2:
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 1
  %eei_1_3 = extractelement <4 x float> %vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %vec1, i64 3
  br label %BB4
BB3:
  %vec2 = call <4 x float> @get_vec4float()
  %eei_2_1 = extractelement <4 x float> %vec2, i64 0
  %eei_2_2 = extractelement <4 x float> %vec2, i64 1
  %eei_2_3 = extractelement <4 x float> %vec2, i64 2
  %eei_2_4 = extractelement <4 x float> %vec2, i64 3
  br label %BB4
BB4:
  %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
  %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
  %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_2_3, %BB3 ]
  %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_2_4, %BB3 ]
  call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
  ret void
}

; Check that we don't merge scalar phis when the vector type is different.

; CHECK-LABEL: define void @f1
; ...
; CHECK: BB4:
; CHECK: %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
; CHECK: %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
; CHECK: %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_3_1, %BB3 ]
; CHECK: %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_3_2, %BB3 ]
; CHECK: call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
; CHECK: ret void

define void @f1(i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %BB1, label %BB3
BB1:
  %inc_vec = phi <4 x float> [ zeroinitializer, %entry ], [ %vec1, %BB1 ]
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  br i1 %cond2, label %BB1, label %BB2
BB2:
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 1
  %eei_1_3 = extractelement <4 x float> %vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %vec1, i64 3
  br label %BB4
BB3:
  %vec2 = call <2 x float> @get_vec2float()
  %vec3 = call <2 x float> @get_vec2float()
  %eei_2_1 = extractelement <2 x float> %vec2, i64 0
  %eei_2_2 = extractelement <2 x float> %vec2, i64 1
  %eei_3_1 = extractelement <2 x float> %vec3, i64 0
  %eei_3_2 = extractelement <2 x float> %vec3, i64 1
  br label %BB4
BB4:
  %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
  %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
  %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_3_1, %BB3 ]
  %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_3_2, %BB3 ]
  call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
  ret void
}

; Check that we do not merge scalar phis when EEI instructions corresponding to the same input index are not in the same BB.

; CHECK-LABEL: define void @f2
; ...
; CHECK: BB4:
; CHECK: %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
; CHECK: %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
; CHECK: %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_2_3, %BB3 ]
; CHECK: %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_2_4, %BB3 ]
; CHECK: call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
; CHECK: ret void

define void @f2(i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %BB1, label %BB3
BB1:
  %inc_vec = phi <4 x float> [ zeroinitializer, %entry ], [ %vec1, %BB1 ]
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 1
  br i1 %cond2, label %BB1, label %BB2
BB2:
  %eei_1_3 = extractelement <4 x float> %vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %vec1, i64 3
  br label %BB4
BB3:
  %vec2 = call <4 x float> @get_vec4float()
  %eei_2_1 = extractelement <4 x float> %vec2, i64 0
  %eei_2_2 = extractelement <4 x float> %vec2, i64 1
  %eei_2_3 = extractelement <4 x float> %vec2, i64 2
  %eei_2_4 = extractelement <4 x float> %vec2, i64 3
  br label %BB4
BB4:
  %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
  %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
  %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_2_3, %BB3 ]
  %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_2_4, %BB3 ]
  call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
  ret void
}

; Check that we merge scalar phis when they are in the different BBs.

; CHECK-LABEL: define void @f3
; ...
; CHECK: BB4:
; CHECK: %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
; CHECK: %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
; CHECK: call void @use_2scalars(float %res_scalar1, float %res_scalar2)
; CHECK: br label %BB6
; CHECK: BB5:
; CHECK: %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_2_3, %BB3 ]
; CHECK: %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_2_4, %BB3 ]
; CHECK: call void @use_2scalars(float %res_scalar3, float %res_scalar4)
; CHECK: br label %BB6
; CHECK: BB6:
; CHECK: ret void

define void @f3(i1 %cond1, i1 %cond2, i1 %cond3) {
entry:
  br i1 %cond1, label %BB1, label %BB3
BB1:
  %inc_vec = phi <4 x float> [ zeroinitializer, %entry ], [ %vec1, %BB1 ]
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  br i1 %cond2, label %BB1, label %BB2
BB2:
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 1
  %eei_1_3 = extractelement <4 x float> %vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %vec1, i64 3
  br i1 %cond3, label %BB4, label %BB5
BB3:
  %vec2 = call <4 x float> @get_vec4float()
  %eei_2_1 = extractelement <4 x float> %vec2, i64 0
  %eei_2_2 = extractelement <4 x float> %vec2, i64 1
  %eei_2_3 = extractelement <4 x float> %vec2, i64 2
  %eei_2_4 = extractelement <4 x float> %vec2, i64 3
   br i1 %cond3, label %BB4, label %BB5
BB4:
  %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
  %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
  call void @use_2scalars(float %res_scalar1, float %res_scalar2)
  br label %BB6
BB5:
  %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_2_3, %BB3 ]
  %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_2_4, %BB3 ]
  call void @use_2scalars(float %res_scalar3, float %res_scalar4)
  br label %BB6
BB6:
  ret void
}

; Check that we merge scalar phis with mores than 2 incoming values.
;
; CHECK-LABEL: define void @f4
; ...
; CHECK-NOT: phi float
; ...
; CHECK: BB1:
; CHECK: %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
; ...
; CHECK: BB3:
; CHECK: %vec2 = call <4 x float> @get_vec4float()
; ...
; CHECK: BB4:
; CHECK: %vec3 = call <4 x float> @get_vec4float()
; ...
; CHECK: BB5:
; CHECK: [[MERGED_PHI:%.*]] = phi <4 x float> [ %vec1, %BB2 ], [ %vec2, %BB3 ], [ %vec3, %BB4 ]
; CHECK: [[NEW_EEI1:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 0
; CHECK: [[NEW_EEI2:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 1
; CHECK: [[NEW_EEI3:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 2
; CHECK: [[NEW_EEI4:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 3
; CHECK: call void @use_4scalars(float [[NEW_EEI1]], float [[NEW_EEI2]], float [[NEW_EEI3]], float [[NEW_EEI4]])
; CHECK: ret void

define void @f4(i1 %cond1, i1 %cond2, i1 %cond3) {
entry:
  br i1 %cond1, label %BB1, label %BB3
BB1:
  %inc_vec = phi <4 x float> [ zeroinitializer, %entry ], [ %vec1, %BB1 ]
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  br i1 %cond2, label %BB1, label %BB2
BB2:
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 1
  %eei_1_3 = extractelement <4 x float> %vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %vec1, i64 3
  br i1 %cond3, label %BB5, label %BB4
BB3:
  %vec2 = call <4 x float> @get_vec4float()
  %eei_2_1 = extractelement <4 x float> %vec2, i64 0
  %eei_2_2 = extractelement <4 x float> %vec2, i64 1
  %eei_2_3 = extractelement <4 x float> %vec2, i64 2
  %eei_2_4 = extractelement <4 x float> %vec2, i64 3
  br label %BB5
BB4:
  %vec3 = call <4 x float> @get_vec4float()
  %eei_3_1 = extractelement <4 x float> %vec3, i64 0
  %eei_3_2 = extractelement <4 x float> %vec3, i64 1
  %eei_3_3 = extractelement <4 x float> %vec3, i64 2
  %eei_3_4 = extractelement <4 x float> %vec3, i64 3
  br label %BB5
BB5:
  %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ], [ %eei_3_1, %BB4 ]
  %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ], [ %eei_3_2, %BB4 ]
  %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_2_3, %BB3 ], [ %eei_3_3, %BB4 ]
  %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_2_4, %BB3 ], [ %eei_3_4, %BB4 ]
  call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
  ret void
}


; Check that we don't merge scalar phis when EEIs indices are not the same across incoming values.
;
; CHECK-LABEL: define void @f5
; CHECK: BB4:
; CHECK: %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
; CHECK: %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
; CHECK: %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_2_3, %BB3 ]
; CHECK: %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_2_4, %BB3 ]
; CHECK: call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
; CHECK: ret void

define void @f5(i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %BB1, label %BB3
BB1:
  %inc_vec = phi <4 x float> [ zeroinitializer, %entry ], [ %vec1, %BB1 ]
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  br i1 %cond2, label %BB1, label %BB2
BB2:
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 0
  %eei_1_3 = extractelement <4 x float> %vec1, i64 0
  %eei_1_4 = extractelement <4 x float> %vec1, i64 0
  br label %BB4
BB3:
  %vec2 = call <4 x float> @get_vec4float()
  %eei_2_1 = extractelement <4 x float> %vec2, i64 0
  %eei_2_2 = extractelement <4 x float> %vec2, i64 1
  %eei_2_3 = extractelement <4 x float> %vec2, i64 2
  %eei_2_4 = extractelement <4 x float> %vec2, i64 3
  br label %BB4
BB4:
  %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_2_1, %BB3 ]
  %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_2_2, %BB3 ]
  %res_scalar3 = phi float [ %eei_1_3, %BB2 ], [ %eei_2_3, %BB3 ]
  %res_scalar4 = phi float [ %eei_1_4, %BB2 ], [ %eei_2_4, %BB3 ]
  call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
  ret void
}


; Check that we merge scalar phis when there incoming values are from diffrent vector values.
;
; CHECK-LABEL: define void @f6
;
; CHECK-NOT: phi float
; ...
; CHECK: BB1:
; CHECK: %vec1 = call <2 x float> @update_vec2float(<2 x float> %inc_vec1)
; CHECK: %vec2 = call <2 x float> @update_vec2float(<2 x float> %inc_vec2)
; ...
; CHECK: BB3:
; CHECK: %vec3 = call <2 x float> @get_vec2float()
; CHECK: %vec4 = call <2 x float> @get_vec2float()
; ...
; CHECK: BB4:
; CHECK: [[MERGED_PHI1:%.*]] = phi <2 x float> [ %vec1, %BB2 ], [ %vec3, %BB3 ]
; CHECK: [[MERGED_PHI2:%.*]] = phi <2 x float> [ %vec2, %BB2 ], [ %vec4, %BB3 ]
; CHECK: [[NEW_EEI3:%.*]] = extractelement <2 x float> [[MERGED_PHI2]], i64 0
; CHECK: [[NEW_EEI4:%.*]] = extractelement <2 x float> [[MERGED_PHI2]], i64 1
; CHECK: [[NEW_EEI1:%.*]] = extractelement <2 x float> [[MERGED_PHI1]], i64 0
; CHECK: [[NEW_EEI2:%.*]] = extractelement <2 x float> [[MERGED_PHI1]], i64 1
; CHECK: call void @use_4scalars(float [[NEW_EEI1]], float [[NEW_EEI2]], float [[NEW_EEI3]], float [[NEW_EEI4]])
; CHECK: ret void

define void @f6(i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %BB1, label %BB3
BB1:
  %inc_vec1 = phi <2 x float> [ zeroinitializer, %entry ], [ %vec1, %BB1 ]
  %inc_vec2 = phi <2 x float> [ zeroinitializer, %entry ], [ %vec2, %BB1 ]
  %vec1 = call <2 x float> @update_vec2float(<2 x float> %inc_vec1)
  %vec2 = call <2 x float> @update_vec2float(<2 x float> %inc_vec2)
  br i1 %cond2, label %BB1, label %BB2
BB2:
  %eei_1_1 = extractelement <2 x float> %vec1, i64 0
  %eei_1_2 = extractelement <2 x float> %vec1, i64 1
  %eei_2_1 = extractelement <2 x float> %vec2, i64 0
  %eei_2_2 = extractelement <2 x float> %vec2, i64 1
  br label %BB4
BB3:
  %vec3 = call <2 x float> @get_vec2float()
  %vec4 = call <2 x float> @get_vec2float()
  %eei_3_1 = extractelement <2 x float> %vec3, i64 0
  %eei_3_2 = extractelement <2 x float> %vec3, i64 1
  %eei_4_1 = extractelement <2 x float> %vec4, i64 0
  %eei_4_2 = extractelement <2 x float> %vec4, i64 1
  br label %BB4
BB4:
  %res_scalar1 = phi float [ %eei_1_1, %BB2 ], [ %eei_3_1, %BB3 ]
  %res_scalar2 = phi float [ %eei_1_2, %BB2 ], [ %eei_3_2, %BB3 ]
  %res_scalar3 = phi float [ %eei_2_1, %BB2 ], [ %eei_4_1, %BB3 ]
  %res_scalar4 = phi float [ %eei_2_2, %BB2 ], [ %eei_4_2, %BB3 ]
  call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
  ret void
}


; Check one incoming value phi node support.
;
; CHECK-LABEL: define void @f7
;
; CHECK-NOT: phi float
; ...
; CHECK: entry:
; CHECK: %vec = call <4 x float> @get_vec4float()
; ...
; CHECK: BB1:
; CHECK: [[MERGED_PHI:%.*]] = phi <4 x float> [ %vec, %entry ]
; CHECK: [[NEW_EEI1:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 0
; CHECK: [[NEW_EEI2:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 1
; CHECK: [[NEW_EEI3:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 2
; CHECK: [[NEW_EEI4:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 3
; CHECK: call void @use_4scalars(float [[NEW_EEI1]], float [[NEW_EEI2]], float [[NEW_EEI3]], float [[NEW_EEI4]])
; CHECK: ret void

define void @f7() {
entry:
  %vec = call <4 x float> @get_vec4float()
  %eei1 = extractelement <4 x float> %vec, i64 0
  %eei2 = extractelement <4 x float> %vec, i64 1
  %eei3 = extractelement <4 x float> %vec, i64 2
  %eei4 = extractelement <4 x float> %vec, i64 3
  br label %BB1
BB1:
  %res_scalar1 = phi float [ %eei1, %entry ]
  %res_scalar2 = phi float [ %eei2, %entry ]
  %res_scalar3 = phi float [ %eei3, %entry ]
  %res_scalar4 = phi float [ %eei4, %entry ]
  call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
  ret void
}

; Check optimization enabler pattern.
;
; CHECK-LABEL: define void @f8
; ...
; CHECK-NOT: phi float
;
; CHECK: BB1:
; CHECK: %vec1 = call <4 x float> @get_vec4float()
; ...
; CHECK BB2:
; CHECK: [[MERGED_PHI1:%.*]] = phi <4 x float> [ %vec1, %BB1 ]
; ...
; CHECK: BB3:
; CHECK: %vec2 = call <4 x float> @get_vec4float()
; ...
; CHECK: BB4:
; CHECK: [[MERGED_PHI2:%.*]] = phi <4 x float> [ [[MERGED_PHI1]], %BB2 ], [ %vec2, %BB3 ]
; CHECK: [[NEW_EEI1:%.*]] = extractelement <4 x float> [[MERGED_PHI2]], i64 0
; CHECK: [[NEW_EEI2:%.*]] = extractelement <4 x float> [[MERGED_PHI2]], i64 1
; CHECK: [[NEW_EEI3:%.*]] = extractelement <4 x float> [[MERGED_PHI2]], i64 2
; CHECK: [[NEW_EEI4:%.*]] = extractelement <4 x float> [[MERGED_PHI2]], i64 3
; CHECK: call void @use_4scalars(float [[NEW_EEI1]], float [[NEW_EEI2]], float [[NEW_EEI3]], float [[NEW_EEI4]])
; CHECK: ret void

define void @f8(i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %BB1, label %BB3
BB1:
  %vec1 = call <4 x float> @get_vec4float()
  %eei1_1 = extractelement <4 x float> %vec1, i64 0
  %eei1_2 = extractelement <4 x float> %vec1, i64 1
  %eei1_3 = extractelement <4 x float> %vec1, i64 2
  %eei1_4 = extractelement <4 x float> %vec1, i64 3
  br label %BB2
BB2:
  %res_scalar1_1 = phi float [ %eei1_1, %BB1 ]
  %res_scalar1_2 = phi float [ %eei1_2, %BB1 ]
  %res_scalar1_3 = phi float [ %eei1_3, %BB1 ]
  %res_scalar1_4 = phi float [ %eei1_4, %BB1 ]
  br label %BB4
BB3:
  %vec2 = call <4 x float> @get_vec4float()
  %eei2_1 = extractelement <4 x float> %vec2, i64 0
  %eei2_2 = extractelement <4 x float> %vec2, i64 1
  %eei2_3 = extractelement <4 x float> %vec2, i64 2
  %eei2_4 = extractelement <4 x float> %vec2, i64 3
  br label %BB4
BB4:
  %res_scalar1 = phi float [ %res_scalar1_1, %BB2 ], [ %eei2_1, %BB3 ]
  %res_scalar2 = phi float [ %res_scalar1_2, %BB2 ], [ %eei2_2, %BB3 ]
  %res_scalar3 = phi float [ %res_scalar1_3, %BB2 ], [ %eei2_3, %BB3 ]
  %res_scalar4 = phi float [ %res_scalar1_4, %BB2 ], [ %eei2_4, %BB3 ]
  call void @use_4scalars(float %res_scalar1, float %res_scalar2, float %res_scalar3, float %res_scalar4)
  ret void
}

; Check that the optimization merges scalar phis with zero incoming values.
; Check that redundant phi nodes are removed.

; CHECK-LABEL: define void @f9
; CHECK: entry:
; CHECK: br label %BB1
; ...
; CHECK: BB1:
; CHECK-NOT: %inc_vec = phi <4 x float> [ zeroinitializer, %entry ], [ %vec1, %BB2 ]
; CHECK: [[MERGED_PHI:%.*]] = phi <4 x float> [ zeroinitializer, %entry ], [ %vec1, %BB2 ]
; CHECK: [[NEW_EEI1:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 0
; CHECK: [[NEW_EEI2:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 1
; CHECK: [[NEW_EEI3:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 2
; CHECK: [[NEW_EEI4:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 3
; CHECK: call void @use_4scalars(float [[NEW_EEI1]], float [[NEW_EEI2]], float [[NEW_EEI3]], float [[NEW_EEI4]])
; ...
; CHECK BB2:
; CHECK: br i1 %cond2, label %BB1, label %BBEXIT
; CHECK: BBEXIT:
; CHECK: ret void

define void @f9(i1 %cond1, i1 %cond2) {
entry:
  br label %BB1
BB1:
  %inc_vec = phi <4 x float> [ zeroinitializer, %entry ], [ %vec1, %BB2 ]
  %phi1 = phi float [ 0.0, %entry ], [ %eei_1_1, %BB2 ]
  %phi2 = phi float [ 0.0, %entry ], [ %eei_1_2, %BB2 ]
  %phi3 = phi float [ 0.0, %entry ], [ %eei_1_3, %BB2 ]
  %phi4 = phi float [ 0.0, %entry ], [ %eei_1_4, %BB2 ]
  call void @use_4scalars(float %phi1, float %phi2, float %phi3, float %phi4)
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  br i1 %cond1, label %BB2, label %BBEXIT
BB2:
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 1
  %eei_1_3 = extractelement <4 x float> %vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %vec1, i64 3
  br i1 %cond2, label %BB1, label %BBEXIT
BBEXIT:
  ret void
}


; Check that the optimization merges scalar phis with zero incoming values.
; Check that redundant phi nodes are removed.

; CHECK-LABEL: define void @f10
; CHECK: entry:
; CHECK: br label %BB1
; ...
; CHECK: BB1:
; CHECK-NOT: %inc_vec = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
; CHECK: [[MERGED_PHI:%.*]] = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
; CHECK: [[NEW_EEI1:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 0
; CHECK: [[NEW_EEI2:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 1
; CHECK: [[NEW_EEI3:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 2
; CHECK: [[NEW_EEI4:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 3
; CHECK: call void @use_4scalars(float [[NEW_EEI1]], float [[NEW_EEI2]], float [[NEW_EEI3]], float [[NEW_EEI4]])
; ...
; CHECK BB2:
; CHECK: br i1 %cond2, label %BB1, label %BBEXIT
; CHECK: BBEXIT:
; CHECK: ret void

define void @f10(i1 %cond1, i1 %cond2) {
entry:
  br label %BB1
BB1:
  %inc_vec = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
  %phi1 = phi float [ %eei_1_1, %BB2 ], [ 0.0, %entry ]
  %phi2 = phi float [ %eei_1_2, %BB2 ], [ 0.0, %entry ]
  %phi3 = phi float [ %eei_1_3, %BB2 ], [ 0.0, %entry ]
  %phi4 = phi float [ %eei_1_4, %BB2 ], [ 0.0, %entry ]
  call void @use_4scalars(float %phi1, float %phi2, float %phi3, float %phi4)
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  br i1 %cond1, label %BB2, label %BBEXIT
BB2:
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 1
  %eei_1_3 = extractelement <4 x float> %vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %vec1, i64 3
  br i1 %cond2, label %BB1, label %BBEXIT
BBEXIT:
  ret void
}

; Check that the optimization is not applied when one of incoming values is not zero.
; CHECK-LABEL: define void @f11
; CHECK: entry:
; CHECK: br label %BB1
; ...
; CHECK: BB1:
; ...
; CHECK: call void @use_4scalars(float %phi1, float %phi2, float %phi3, float %phi4)
; CHECK: %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
; ...
; CHECK BB2:
; CHECK: %eei_1_1 = extractelement <4 x float> %vec1, i64 0
; CHECK: %eei_1_2 = extractelement <4 x float> %vec1, i64 1
; CHECK: %eei_1_3 = extractelement <4 x float> %vec1, i64 2
; CHECK: %eei_1_4 = extractelement <4 x float> %vec1, i64 3
; CHECK: br i1 %cond2, label %BB1, label %BBEXIT
; CHECK: BBEXIT:
; CHECK: ret void

define void @f11(i1 %cond1, i1 %cond2, float %val) {
entry:
  br label %BB1
BB1:
  %inc_vec = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
  %phi1 = phi float [ %eei_1_1, %BB2 ], [ 0.0, %entry ]
  %phi2 = phi float [ %eei_1_2, %BB2 ], [ %val, %entry ]
  %phi3 = phi float [ %eei_1_3, %BB2 ], [ 0.0, %entry ]
  %phi4 = phi float [ %eei_1_4, %BB2 ], [ 0.0, %entry ]
  call void @use_4scalars(float %phi1, float %phi2, float %phi3, float %phi4)
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  br i1 %cond1, label %BB2, label %BBEXIT
BB2:
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 1
  %eei_1_3 = extractelement <4 x float> %vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %vec1, i64 3
  br i1 %cond2, label %BB1, label %BBEXIT
BBEXIT:
  ret void
}

; Check that the optimization is applied when extractelement have several users.

; CHECK-LABEL: define void @f12
; CHECK: entry:
; CHECK: br label %BB1
; ...
; CHECK: BB1:
; CHECK-NOT: %inc_vec = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
; CHECK: [[MERGED_PHI:%.*]] = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
; CHECK: [[NEW_EEI1:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 0
; CHECK: [[NEW_EEI2:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 1
; CHECK: [[NEW_EEI3:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 2
; CHECK: [[NEW_EEI4:%.*]] = extractelement <4 x float> [[MERGED_PHI]], i64 3
; CHECK: call void @use_4scalars(float [[NEW_EEI1]], float [[NEW_EEI2]], float [[NEW_EEI3]], float [[NEW_EEI4]])
; CHECK: %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
; CHECK: br i1 %cond1, label %BB2, label %BBEXIT
; ...
; CHECK BB2:
; CHECK-NOT: %eei_1_1 = extractelement <4 x float> %vec1, i64 0
; CHECK: %eei_1_2 = extractelement <4 x float> %vec1, i64 1
; CHECK: call void @use_float(float %eei_1_2)
; CHECK-NOT: %eei_1_3 = extractelement <4 x float> %vec1, i64 2
; CHECK-NOT: %eei_1_4 = extractelement <4 x float> %vec1, i64 3
; CHECK: br i1 %cond2, label %BB1, label %BBEXIT
; CHECK: BBEXIT:
; CHECK: ret void
define void @f12(i1 %cond1, i1 %cond2) {
entry:
  br label %BB1
BB1:
  %inc_vec = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
  %phi1 = phi float [ %eei_1_1, %BB2 ], [ 0.0, %entry ]
  %phi2 = phi float [ %eei_1_2, %BB2 ], [ 0.0, %entry ]
  %phi3 = phi float [ %eei_1_3, %BB2 ], [ 0.0, %entry ]
  %phi4 = phi float [ %eei_1_4, %BB2 ], [ 0.0, %entry ]
  call void @use_4scalars(float %phi1, float %phi2, float %phi3, float %phi4)
  %vec1 = call <4 x float> @update_vec4float(<4 x float> %inc_vec)
  br i1 %cond1, label %BB2, label %BBEXIT
BB2:
  %eei_1_1 = extractelement <4 x float> %vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %vec1, i64 1
  call void @use_float(float %eei_1_2)
  %eei_1_3 = extractelement <4 x float> %vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %vec1, i64 3
  br i1 %cond2, label %BB1, label %BBEXIT
BBEXIT:
  ret void
}

; Check redundant phi nodes removal.
; CHECK-LABEL: define void @f13
; CHECK: entry:
; CHECK: br label %BB1
; CHECK: BB1:
; CHECK-NOT: %inc_vec2 = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
; CHECK-NOT: %inc_vec3 = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
; CHECK-NOT: %inc_vec4 = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]

define void @f13(i1 %cond1, i1 %cond2) {
entry:
  br label %BB1
BB1:
  %inc_vec1 = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
  %inc_vec2 = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
  %inc_vec3 = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
  %inc_vec4 = phi <4 x float> [ %vec1, %BB2 ], [ zeroinitializer, %entry ]
  %phi1_1 = phi float [ %eei_1_1, %BB2 ], [ 0.0, %entry ]
  %phi1_2 = phi float [ %eei_1_2, %BB2 ], [ 0.0, %entry ]
  %phi1_3 = phi float [ %eei_1_3, %BB2 ], [ 0.0, %entry ]
  %phi1_4 = phi float [ %eei_1_4, %BB2 ], [ 0.0, %entry ]
  %phi2_1 = phi float [ %eei_2_1, %BB2 ], [ 0.0, %entry ]
  %phi2_2 = phi float [ %eei_2_2, %BB2 ], [ 0.0, %entry ]
  %phi2_3 = phi float [ %eei_2_3, %BB2 ], [ 0.0, %entry ]
  %phi2_4 = phi float [ %eei_2_4, %BB2 ], [ 0.0, %entry ]
  %phi3_1 = phi float [ %eei_3_1, %BB2 ], [ 0.0, %entry ]
  %phi3_2 = phi float [ %eei_3_2, %BB2 ], [ 0.0, %entry ]
  %phi3_3 = phi float [ %eei_3_3, %BB2 ], [ 0.0, %entry ]
  %phi3_4 = phi float [ %eei_3_4, %BB2 ], [ 0.0, %entry ]
  %phi4_1 = phi float [ %eei_4_1, %BB2 ], [ 0.0, %entry ]
  %phi4_2 = phi float [ %eei_4_2, %BB2 ], [ 0.0, %entry ]
  %phi4_3 = phi float [ %eei_4_3, %BB2 ], [ 0.0, %entry ]
  %phi4_4 = phi float [ %eei_4_4, %BB2 ], [ 0.0, %entry ]
  call void @use_4scalars(float %phi1_1, float %phi1_2, float %phi1_3, float %phi1_4)
  call void @use_4scalars(float %phi2_1, float %phi2_2, float %phi2_3, float %phi2_4)
  call void @use_4scalars(float %phi3_1, float %phi3_2, float %phi3_3, float %phi3_4)
  call void @use_4scalars(float %phi4_1, float %phi4_2, float %phi4_3, float %phi4_4)
  call void @use_vec4float(<4 x float> %inc_vec1)
  call void @use_vec4float(<4 x float> %inc_vec2)
  call void @use_vec4float(<4 x float> %inc_vec3)
  call void @use_vec4float(<4 x float> %inc_vec4)
  %vec1 = call <4 x float> @get_vec4float()
  br i1 %cond1, label %BB2, label %BBEXIT
BB2:
  %eei_1_1 = extractelement <4 x float> %inc_vec1, i64 0
  %eei_1_2 = extractelement <4 x float> %inc_vec1, i64 1
  %eei_1_3 = extractelement <4 x float> %inc_vec1, i64 2
  %eei_1_4 = extractelement <4 x float> %inc_vec1, i64 3
  %eei_2_1 = extractelement <4 x float> %inc_vec2, i64 0
  %eei_2_2 = extractelement <4 x float> %inc_vec2, i64 1
  %eei_2_3 = extractelement <4 x float> %inc_vec2, i64 2
  %eei_2_4 = extractelement <4 x float> %inc_vec2, i64 3
  %eei_3_1 = extractelement <4 x float> %inc_vec3, i64 0
  %eei_3_2 = extractelement <4 x float> %inc_vec3, i64 1
  %eei_3_3 = extractelement <4 x float> %inc_vec3, i64 2
  %eei_3_4 = extractelement <4 x float> %inc_vec3, i64 3
  %eei_4_1 = extractelement <4 x float> %inc_vec4, i64 0
  %eei_4_2 = extractelement <4 x float> %inc_vec4, i64 1
  %eei_4_3 = extractelement <4 x float> %inc_vec4, i64 2
  %eei_4_4 = extractelement <4 x float> %inc_vec4, i64 3
  br i1 %cond2, label %BB1, label %BBEXIT
BBEXIT:
  ret void
}

!igc.functions = !{!0, !3, !5, !7, !9, !11, !13, !15, !17, !19, !21, !23, !25, !27}

!0 = !{void (i1, i1)* @f0, !1}
!3 = !{void (i1, i1)* @f1, !1}
!5 = !{void (i1, i1)* @f2, !1}
!7 = !{void (i1, i1, i1)* @f3, !1}
!9 = !{void (i1, i1, i1)* @f4, !1}
!11 = !{void (i1, i1)* @f5, !1}
!13 = !{void (i1, i1)* @f6, !1}
!15 = !{void ()* @f7, !1}
!17 = !{void (i1, i1)* @f8, !1}
!19 = !{void (i1, i1)* @f9, !1}
!21 = !{void (i1, i1)* @f10, !1}
!23 = !{void (i1, i1, float)* @f11, !1}
!25 = !{void (i1, i1)* @f12, !1}
!27 = !{void (i1, i1)* @f13, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}