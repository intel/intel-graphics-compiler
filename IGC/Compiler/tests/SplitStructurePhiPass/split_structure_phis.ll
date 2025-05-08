;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers %s -S -o - -split-structure-phis | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

declare <64 x float> @external_func_vec64float(<64 x float>)
declare float @external_func_float(float)
declare double @external_func_double(double)
declare i16 @external_func_i16(i16)
declare <32 x i32> @external_func_vec32i32(<32 x i32>)
declare <5 x double> @external_func_vec5double(<5 x double>)
declare void @struct_use_func({ <64 x float>, <64 x float> })
declare void @vector_use_func(<64 x float>)


; This test case verifies that a phi instruction with a structure type with vectors must be split into two phis vectors.

; CHECK-LABEL: define void @f0
; ...
; CHECK: bb1:
; CHECK: [[NEWPHI1:%.*]] = phi <64 x float> [ zeroinitializer, %entry ], [ [[VRES1:%.*]], %bb1 ]
; CHECK: [[NEWPHI2:%.*]] = phi <64 x float> [ zeroinitializer, %entry ], [ [[VRES2:%.*]], %bb1 ]
; CHECK: [[VRES1]] = call <64 x float> @external_func_vec64float(<64 x float> [[NEWPHI1]])
; CHECK: [[VRES2]] = call <64 x float> @external_func_vec64float(<64 x float> [[NEWPHI2]])
; ...
; CHECK: terminator:
; CHECK: store <64 x float> [[VRES1]], ptr %ptr1, align 256
; CHECK: store <64 x float> [[VRES2]], ptr %ptr2, align 256

define void @f0(i32 %val, ptr %ptr1, ptr %ptr2) {
entry:
  br label %bb1
bb1:
  %phinode = phi { <64 x float>, <64 x float> } [ zeroinitializer, %entry ], [ %insert.2, %bb1 ]
  %extract.1 = extractvalue { <64 x float>, <64 x float> } %phinode, 0
  %extract.2 = extractvalue { <64 x float>, <64 x float> } %phinode, 1

  %vector.res.1 = call <64 x float> @external_func_vec64float(<64 x float> %extract.1)
  %vector.res.2 = call <64 x float> @external_func_vec64float(<64 x float> %extract.2)

  %insert.1 = insertvalue { <64 x float>, <64 x float> } poison, <64 x float> %vector.res.1, 0
  %insert.2 = insertvalue { <64 x float>, <64 x float> } %insert.1, <64 x float> %vector.res.2, 1
  %cond = icmp ult i32 %val, 63
  br i1 %cond, label %bb1, label %terminator
terminator:
  store <64 x float> %vector.res.1, ptr %ptr1, align 256
  store <64 x float> %vector.res.2, ptr %ptr2, align 256
  ret void
}

; This test case verifies that the phi instruction with structure type with vectors and scalar is split.

; CHECK-LABEL: define void @f1
; ...
; CHECK: bb1:
; CHECK: [[NEWPHI1:%.*]] = phi <64 x float> [ zeroinitializer, %entry ], [ [[VRES1:%.*]], %bb1 ]
; CHECK: [[NEWPHI2:%.*]] = phi <64 x float> [ zeroinitializer, %entry ], [ [[VRES2:%.*]], %bb1 ]
; CHECK: [[NEWPHI3:%.*]] = phi float [ 0.000000e+00, %entry ], [ [[SRES:%.*]], %bb1 ]
; CHECK: [[VRES1]] = call <64 x float> @external_func_vec64float(<64 x float> [[NEWPHI1]])
; CHECK: [[VRES2]] = call <64 x float> @external_func_vec64float(<64 x float> [[NEWPHI2]])
; CHECK: [[SRES]] = call float @external_func_float(float [[NEWPHI3]])
; ...
; CHECK: terminator:
; CHECK: store <64 x float> [[VRES1]], ptr %ptr1, align 256
; CHECK: store <64 x float> [[VRES2]], ptr %ptr2, align 256
; CHECK: store float [[SRES]], ptr %ptr3, align 4

define void @f1(i32 %val, ptr %ptr1, ptr %ptr2, ptr %ptr3) {
entry:
  br label %bb1
bb1:
  %phinode = phi { <64 x float>, <64 x float>, float } [ zeroinitializer , %entry ], [ %insert.3, %bb1 ]
  %extract.vec1 = extractvalue { <64 x float>, <64 x float>, float } %phinode, 0
  %extract.vec2 = extractvalue { <64 x float>, <64 x float>, float } %phinode, 1
  %extract.scalar = extractvalue { <64 x float>, <64 x float>, float } %phinode, 2

  %vector.res.1 = call <64 x float> @external_func_vec64float(<64 x float> %extract.vec1)
  %vector.res.2 = call <64 x float> @external_func_vec64float(<64 x float> %extract.vec2)
  %scalar.res = call float @external_func_float(float %extract.scalar)

  %insert.1 = insertvalue { <64 x float>, <64 x float>, float } poison, <64 x float> %vector.res.1, 0
  %insert.2 = insertvalue { <64 x float>, <64 x float>, float } %insert.1, <64 x float> %vector.res.2, 1
  %insert.3 = insertvalue { <64 x float>, <64 x float>, float } %insert.2, float %scalar.res, 2
  %cond = icmp ult i32 %val, 63
  br i1 %cond, label %bb1, label %terminator
terminator:
  store <64 x float> %vector.res.1, ptr %ptr1, align 256
  store <64 x float> %vector.res.2, ptr %ptr2, align 256
  store float %scalar.res, ptr %ptr3, align 4
  ret void
}

; This test case verifies that the phi instruction with structure type with vectors and scalars is split and uses are
; correctly replaced.

; CHECK-LABEL: define void @f2
; ...
; CHECK: bb1:
; CHECK: [[NEWPHI1:%.*]] = phi <64 x float> [ zeroinitializer, %entry ], [ [[VRES1:%.*]], %bb1 ]
; CHECK: [[NEWPHI2:%.*]] = phi <64 x float> [ zeroinitializer, %entry ], [ [[VRES2:%.*]], %bb1 ]
; CHECK: [[NEWPHI3:%.*]] = phi float [ 0.000000e+00, %entry ], [ [[SRES1:%.*]], %bb1 ]
; CHECK: [[NEWPHI4:%.*]] = phi float [ 0.000000e+00, %entry ], [ [[SRES2:%.*]], %bb1 ]

; CHECK: [[VRES1]] = call <64 x float> @external_func_vec64float(<64 x float> [[NEWPHI1]])
; CHECK: [[VRES2]] = call <64 x float> @external_func_vec64float(<64 x float> [[NEWPHI2]])
; CHECK: [[SRES1]] = call float @external_func_float(float [[NEWPHI3]])
; CHECK: [[SRES2]] = call float @external_func_float(float [[NEWPHI4]])
; ...
; CHECK: terminator:
; CHECK: store <64 x float> [[VRES1]], ptr %ptr1, align 256
; CHECK: store <64 x float> [[VRES2]], ptr %ptr2, align 256
; CHECK: store float [[SRES1]], ptr %ptr3, align 4
; CHECK: store float [[SRES2]], ptr %ptr4, align 4

define void @f2(i32 %val, ptr %ptr1, ptr %ptr2, ptr %ptr3, ptr %ptr4) {
entry:
  br label %bb1
bb1:
  %phinode = phi { <64 x float>, <64 x float>, float, float } [ zeroinitializer, %entry ], [ %insert.4, %bb1 ]
  %extract.vec1 = extractvalue { <64 x float>, <64 x float>, float, float } %phinode, 0
  %extract.vec2 = extractvalue { <64 x float>, <64 x float>, float, float } %phinode, 1
  %extract.scalar1 = extractvalue { <64 x float>, <64 x float>, float, float } %phinode, 2
  %extract.scalar2 = extractvalue { <64 x float>, <64 x float>, float, float } %phinode, 3

  %vector.res.1 = call <64 x float> @external_func_vec64float(<64 x float> %extract.vec1)
  %vector.res.2 = call <64 x float> @external_func_vec64float(<64 x float> %extract.vec2)
  %scalar.res.1 = call float @external_func_float(float %extract.scalar1)
  %scalar.res.2 = call float @external_func_float(float %extract.scalar2)

  %insert.1 = insertvalue { <64 x float>, <64 x float>, float, float } poison, <64 x float> %vector.res.1, 0
  %insert.2 = insertvalue { <64 x float>, <64 x float>, float, float } %insert.1, <64 x float> %vector.res.2, 1
  %insert.3 = insertvalue { <64 x float>, <64 x float>, float, float } %insert.2, float %scalar.res.1, 2
  %insert.4 = insertvalue { <64 x float>, <64 x float>, float, float } %insert.3, float %scalar.res.2, 3
  %cond = icmp ult i32 %val, 63
  br i1 %cond, label %bb1, label %terminator
terminator:
  store <64 x float> %vector.res.1, ptr %ptr1, align 256
  store <64 x float> %vector.res.2, ptr %ptr2, align 256
  store float %scalar.res.1, ptr %ptr3, align 4
  store float %scalar.res.2, ptr %ptr4, align 4
  ret void
}

; This test case verifies that the phi instruction with structure type with vectors and scalars of different types is split, uses are
; correctly replaced, initialization was done with zeroes.

; CHECK-LABEL: define void @f3
; ...
; CHECK: bb1:
; CHECK: [[NEWPHI1:%.*]] = phi double [ 0.000000e+00, %entry ], [ [[SRES1:%.*]], %bb1 ]
; CHECK: [[NEWPHI2:%.*]] = phi <32 x i32> [ zeroinitializer, %entry ], [ [[VRES1:%.*]], %bb1 ]
; CHECK: [[NEWPHI3:%.*]] = phi i16 [ 0, %entry ], [ [[SRES2:%.*]], %bb1 ]
; CHECK: [[NEWPHI4:%.*]] = phi <5 x double> [ zeroinitializer, %entry ], [ [[VRES2:%.*]], %bb1 ]

; CHECK: [[SRES1]] = call double @external_func_double(double [[NEWPHI1]])
; CHECK: [[VRES1]] = call <32 x i32> @external_func_vec32i32(<32 x i32> [[NEWPHI2]])
; CHECK: [[SRES2]] = call i16 @external_func_i16(i16 [[NEWPHI3]])
; CHECK: [[VRES2]] = call <5 x double> @external_func_vec5double(<5 x double> [[NEWPHI4]])
; ...
; CHECK: terminator:
; CHECK: store double [[SRES1]], ptr %ptr1, align 8
; CHECK: store <32 x i32> [[VRES1]], ptr %ptr2, align 128
; CHECK: store i16 [[SRES2]], ptr %ptr3, align 2
; CHECK: store <5 x double> [[VRES2]], ptr %ptr4, align 32

define void @f3(i32 %val, ptr %ptr1, ptr %ptr2, ptr %ptr3, ptr %ptr4) {
entry:
  br label %bb1
bb1:
  %phinode = phi { double, <32 x i32>, i16, <5 x double> }  [ zeroinitializer, %entry ], [ %insert.4, %bb1 ]
  %extract.double = extractvalue { double, <32 x i32>, i16, <5 x double> } %phinode, 0
  %extract.vec32 = extractvalue { double, <32 x i32>, i16, <5 x double> } %phinode, 1
  %extract.i16 = extractvalue { double, <32 x i32>, i16, <5 x double> } %phinode, 2
  %extract.vec5 = extractvalue { double, <32 x i32>, i16, <5 x double> } %phinode, 3

  %double.res = call double @external_func_double(double %extract.double)
  %vec32.res = call <32 x i32> @external_func_vec32i32(<32 x i32> %extract.vec32)
  %i16.res = call i16 @external_func_i16(i16 %extract.i16)
  %vec5.res = call <5 x double> @external_func_vec5double(<5 x double> %extract.vec5)

  %insert.1 = insertvalue { double, <32 x i32>, i16, <5 x double> } poison, double %double.res, 0
  %insert.2 = insertvalue { double, <32 x i32>, i16, <5 x double> } %insert.1, <32 x i32> %vec32.res, 1
  %insert.3 = insertvalue { double, <32 x i32>, i16, <5 x double> } %insert.2, i16 %i16.res, 2
  %insert.4 = insertvalue { double, <32 x i32>, i16, <5 x double> } %insert.3, <5 x double> %vec5.res, 3
  %cond = icmp ult i32 %val, 63
  br i1 %cond, label %bb1, label %terminator
terminator:
  store double %double.res, ptr %ptr1, align 8
  store <32 x i32> %vec32.res, ptr %ptr2, align 128
  store i16 %i16.res, ptr %ptr3, align 2
  store <5 x double> %vec5.res, ptr %ptr4, align 32
  ret void
}

; This test case verifies that the phi node isntruction is not split if the structure is used in its entirety somewhere, rather than using individual fields of it.

; CHECK-LABEL: define void @f4
; ...
; CHECK-NOT: phi <64 x float>
; CHECK: %phinode = phi { <64 x float>, <64 x float> }

define void @f4(i32 %val, ptr %ptr1, ptr %ptr2) {
entry:
  br label %bb1
bb1:
  %phinode = phi { <64 x float>, <64 x float> } [ zeroinitializer, %entry ], [ %insert.2, %bb1 ]
  %extract.1 = extractvalue { <64 x float>, <64 x float> } %phinode, 0
  %extract.2 = extractvalue { <64 x float>, <64 x float> } %phinode, 1

  %vector.res.1 = call <64 x float> @external_func_vec64float(<64 x float> %extract.1)
  %vector.res.2 = call <64 x float> @external_func_vec64float(<64 x float> %extract.2)
  call void @struct_use_func({ <64 x float>, <64 x float> } %phinode)

  %insert.1 = insertvalue { <64 x float>, <64 x float> } poison, <64 x float> %vector.res.1, 0
  %insert.2 = insertvalue { <64 x float>, <64 x float> } %insert.1, <64 x float> %vector.res.2, 1
  %cond = icmp ult i32 %val, 63
  br i1 %cond, label %bb1, label %terminator
terminator:
  store <64 x float> %vector.res.1, ptr %ptr1, align 256
  store <64 x float> %vector.res.2, ptr %ptr2, align 256
  ret void
}

; This test case verifies that the phi node isntruction is not split if the result of insertvalue instructions that build the structure are not used
; only in the phi instruction.

; CHECK-LABEL: define void @f5
; ...
; CHECK-NOT: phi <64 x float>
; CHECK: %phinode = phi { <64 x float>, <64 x float> }

define void @f5(i32 %val, ptr %ptr1, ptr %ptr2) {
entry:
  br label %bb1
bb1:
  %phinode = phi { <64 x float>, <64 x float> } [ zeroinitializer, %entry ], [ %insert.2, %bb1 ]
  %extract.1 = extractvalue { <64 x float>, <64 x float> } %phinode, 0
  %extract.2 = extractvalue { <64 x float>, <64 x float> } %phinode, 1

  %vector.res.1 = call <64 x float> @external_func_vec64float(<64 x float> %extract.1)
  %vector.res.2 = call <64 x float> @external_func_vec64float(<64 x float> %extract.2)

  %insert.1 = insertvalue { <64 x float>, <64 x float> } poison, <64 x float> %vector.res.1, 0
  %insert.2 = insertvalue { <64 x float>, <64 x float> } %insert.1, <64 x float> %vector.res.2, 1
  %cond = icmp ult i32 %val, 63
  br i1 %cond, label %bb1, label %terminator
terminator:
  call void @struct_use_func({ <64 x float>, <64 x float> } %insert.2)
  store <64 x float> %vector.res.1, ptr %ptr1, align 256
  store <64 x float> %vector.res.2, ptr %ptr2, align 256
  ret void
}


!igc.functions = !{!0, !3, !5, !7, !9, !11}

!0 = !{void (i32, ptr, ptr)* @f0, !1}
!3 = !{void (i32, ptr, ptr, ptr)* @f1, !1}
!5 = !{void (i32, ptr, ptr, ptr, ptr)* @f2, !1}
!7 = !{void (i32, ptr, ptr, ptr, ptr)* @f3, !1}
!9 = !{void (i32, ptr, ptr)* @f4, !1}
!11 = !{void (i32, ptr, ptr)* @f5, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}