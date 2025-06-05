;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK: %"struct::joint_matrix::C.resolved" = type { <8 x float> }
%"struct::joint_matrix::C" = type { target("spirv.JointMatrixINTEL", float, 8, 16, 3, 3, 2) }

; CHECK: %"struct::joint_matrix::A.resolved" = type { <8 x i16> }
%"struct::joint_matrix::A" = type { target("spirv.JointMatrixINTEL", i16, 8, 16, 0, 3, 0) }

; CHECK: %"struct::joint_matrix::B.resolved" = type { <8 x i32> }
%"struct::joint_matrix::B" = type { target("spirv.JointMatrixINTEL", i16, 16, 16, 2, 3, 1) }

; CHECK: %"struct::almost_joint_matrix" = type { ptr }
%"struct::almost_joint_matrix" = type { ptr }

; CHECK-LABEL: define spir_kernel void @test(
; CHECK-SAME: i64 [[OFFSET:%.*]], ptr [[PTR:%.*]], ptr [[PTR1:%.*]]) {
define spir_kernel void @test(i64 %offset, ptr %ptr, ptr %ptr1) {

; CHECK-NEXT:    [[TC_I1:%.*]] = alloca [4 x [4 x %"struct::joint_matrix::C.resolved"]], align 8
; CHECK-NEXT:    [[TA_I3:%.*]] = alloca [4 x [2 x %"struct::joint_matrix::A.resolved"]], align 8
; CHECK-NEXT:    [[TB_I5:%.*]] = alloca [4 x [2 x %"struct::joint_matrix::B.resolved"]], align 8
; CHECK-NEXT:    [[TI_I:%.*]] = alloca [4 x [4 x %"struct::almost_joint_matrix"]], align 8
; CHECK-NEXT:    [[GROUPID:%.*]] = alloca [3 x i64], align 8
  %tC.i = alloca [4 x [4 x %"struct::joint_matrix::C"]], align 8
  %tA.i = alloca [4 x [2 x %"struct::joint_matrix::A"]], align 8
  %tB.i = alloca [4 x [2 x %"struct::joint_matrix::B"]], align 8
  %tI.i = alloca [4 x [4 x %"struct::almost_joint_matrix"]], align 8
  %GroupID = alloca [3 x i64], align 8

; Update lifetime.start size
; CHECK-NEXT:    call void @llvm.lifetime.start.p0(i64 512, ptr [[TC_I1]])
; CHECK-NEXT:    call void @llvm.lifetime.start.p0(i64 128, ptr [[TA_I3]])
; CHECK-NEXT:    call void @llvm.lifetime.start.p0(i64 256, ptr [[TB_I5]])
  call void @llvm.lifetime.start.p0i8(i64 128, ptr %tC.i)
  call void @llvm.lifetime.start.p0i8(i64 64, ptr %tA.i)
  call void @llvm.lifetime.start.p0i8(i64 64, ptr %tB.i)

; Update GEP offsets
; CHECK-NEXT:    [[I1:%.*]] = getelementptr inbounds i8, ptr [[TC_I1]], i64 128
; CHECK-NEXT:    [[I2:%.*]] = getelementptr inbounds i8, ptr [[TA_I3]], i64 64
; CHECK-NEXT:    [[I3:%.*]] = getelementptr inbounds i8, ptr [[TB_I5]], i64 64
  %i1 = getelementptr inbounds i8, ptr %tC.i, i64 128
  %i2 = getelementptr inbounds i8, ptr %tA.i, i64 64
  %i3 = getelementptr inbounds i8, ptr %tB.i, i64 64

; Do not touch if offest is not a constant
; CHECK-NEXT:    [[I4:%.*]] = getelementptr inbounds i8, ptr [[TC_I1]], i64 [[OFFSET]]
  %i4 = getelementptr inbounds i8, ptr %tC.i, i64 %offset

; no change: GEP operand is not a result of bitcast
; CHECK-NEXT:    [[I5:%.*]] = getelementptr inbounds i8, ptr [[PTR1]], i64 128
  %i5 = getelementptr inbounds i8, ptr %ptr1, i64 128

; Do not touch if bitcast is not for matrix type
; CHECK-NEXT:    [[I6:%.*]] = getelementptr inbounds i8, ptr [[TI_I]], i64 128
  %i6 = getelementptr inbounds i8, ptr %tI.i, i64 128

; no change - GEP is not based on i8
; CHECK-NEXT:    [[ARRAYCTOR_END_I:%.*]] = getelementptr inbounds i16, ptr [[TC_I1]], i64 128
  %arrayctor.end.i = getelementptr inbounds i16, ptr  %tC.i, i64 128

; Life time end size update
; CHECK-NEXT:    call void @llvm.lifetime.end.p0(i64 256, ptr [[TB_I5]])
; CHECK-NEXT:    call void @llvm.lifetime.end.p0(i64 128, ptr [[TA_I3]])
; CHECK-NEXT:    call void @llvm.lifetime.end.p0(i64 512, ptr [[TC_I1]])
  call void @llvm.lifetime.end.p0i8(i64 64, ptr %tB.i)
  call void @llvm.lifetime.end.p0i8(i64 64, ptr %tA.i)
  call void @llvm.lifetime.end.p0i8(i64 128, ptr %tC.i)

; do not touch life time intrinsics if not for Joint Matrix types
; CHECK-NEXT:    [[GROUPID_ASCAST:%.*]] = addrspacecast ptr [[GROUPID]] to ptr addrspace(4)
; CHECK-NEXT:    call void @llvm.lifetime.start.p0(i64 24, ptr [[GROUPID]])
; CHECK-NEXT:    call void @llvm.lifetime.end.p0(i64 24, ptr [[GROUPID]])
  %GroupID.ascast = addrspacecast ptr %GroupID to ptr addrspace(4)
  call void @llvm.lifetime.start.p0i8(i64 24, ptr %GroupID)
  call void @llvm.lifetime.end.p0i8(i64 24, ptr %GroupID)

; CHECK-NEXT:    ret void
  ret void
}

declare void @llvm.lifetime.start.p0i8(i64 immarg, ptr nocapture)
declare void @llvm.lifetime.end.p0i8(i64 immarg, ptr nocapture)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
